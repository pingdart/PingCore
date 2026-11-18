#include "web_dev_server.h"

#include <pingcore/route_compiler.h>
#include <pingcore/server.h>
#include <pingdart/web/url_security.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#ifndef PINGCORE_SOURCE_DIR
#define PINGCORE_SOURCE_DIR ""
#endif

#ifndef PINGCORE_BUILD_DIR
#define PINGCORE_BUILD_DIR ""
#endif

namespace {

volatile std::sig_atomic_t stop_requested = 0;

void request_stop(int) {
    stop_requested = 1;
}

std::string quote_shell_argument(const std::string& value) {
#if defined(_WIN32)
    std::string quoted{"\""};
    for (char c : value) {
        if (c == '"') quoted.push_back('\\');
        quoted.push_back(c);
    }
    quoted.push_back('"');
    return quoted;
#else
    std::string quoted{"'"};
    for (char c : value) {
        if (c == '\'') {
            quoted.append("'\\''");
        } else {
            quoted.push_back(c);
        }
    }
    quoted.push_back('\'');
    return quoted;
#endif
}

bool is_cpp_source(const std::filesystem::path& path) {
    const auto extension = path.extension().string();
    return extension == ".c" || extension == ".cc" || extension == ".cpp" ||
           extension == ".cxx" || extension == ".h" || extension == ".hh" ||
           extension == ".hpp" || path.filename() == "CMakeLists.txt";
}

using FileSnapshot = std::map<std::string, std::pair<std::filesystem::file_time_type, uintmax_t>>;

FileSnapshot take_snapshot(const std::vector<std::filesystem::path>& roots) {
    FileSnapshot snapshot;
    std::error_code ec;

    for (const auto& root : roots) {
        if (!std::filesystem::exists(root, ec)) {
            ec.clear();
            continue;
        }

        if (std::filesystem::is_regular_file(root, ec)) {
            const auto time = std::filesystem::last_write_time(root, ec);
            if (!ec && is_cpp_source(root)) {
                const auto size = std::filesystem::file_size(root, ec);
                if (!ec) snapshot.emplace(root.generic_string(), std::make_pair(time, size));
            }
            ec.clear();
            continue;
        }

        std::filesystem::recursive_directory_iterator it(
            root, std::filesystem::directory_options::skip_permission_denied, ec);
        const std::filesystem::recursive_directory_iterator end;
        while (!ec && it != end) {
            if (it->is_regular_file(ec) && !ec && is_cpp_source(it->path())) {
                const auto time = it->last_write_time(ec);
                if (!ec) {
                    const auto size = it->file_size(ec);
                    if (!ec) snapshot.emplace(it->path().generic_string(), std::make_pair(time, size));
                }
            }
            ec.clear();
            it.increment(ec);
            if (ec) ec.clear();
        }
    }

    return snapshot;
}

std::string with_live_reload(std::string html) {
    static constexpr std::string_view script = R"(
<script>
(() => {
  let version = null;
  const check = async () => {
    try {
      const response = await fetch('/__pingdart/revision', { cache: 'no-store' });
      if (!response.ok) return;
      const next = await response.text();
      if (version !== null && next !== version) location.reload();
      version = next;
    } catch (error) {
      console.error('[PingDart] Live-reload check failed:', error);
    }
  };
  check();
  setInterval(check, 500);
})();
</script>
)";

    const auto body_end = html.rfind("</body>");
    if (body_end == std::string::npos) {
        html.append(script);
    } else {
        html.insert(body_end, script);
    }
    return html;
}

bool build_and_render(
    const std::filesystem::path& build_dir,
    const std::string& target,
    const std::filesystem::path& executable,
    const std::filesystem::path& html_file,
    std::string& html) {
    const std::string build_command =
        "cmake --build " + quote_shell_argument(build_dir.string()) +
        " --target " + target;
    std::cout << "[pd dev] Building " << target << "..." << std::endl;
    if (std::system(build_command.c_str()) != 0) {
        std::cerr << "[pd dev] Build failed; keeping the last working page." << std::endl;
        return false;
    }

    const std::string render_command =
        quote_shell_argument(executable.string()) + " --render-html > " +
        quote_shell_argument(html_file.string());
    if (std::system(render_command.c_str()) != 0) {
        std::cerr << "[pd dev] Example rendering failed; keeping the last working page." << std::endl;
        return false;
    }

    std::ifstream input(html_file, std::ios::binary);
    if (!input) {
        std::cerr << "[pd dev] Could not read rendered HTML: " << html_file << std::endl;
        return false;
    }

    std::string rendered(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());
    if (rendered.empty()) {
        std::cerr << "[pd dev] Example produced empty HTML; keeping the last working page." << std::endl;
        return false;
    }

    html = with_live_reload(std::move(rendered));
    std::cout << "[pd dev] Page rebuilt successfully." << std::endl;
    return true;
}

pingdart::core::Status listen_with_port_fallback(
    pingcore::Server& server,
    uint16_t requested_port) {
    auto status = server.listen(requested_port, false, "127.0.0.1");
    if (status.ok() || requested_port == 0 ||
        !status.message().starts_with("Failed to bind to port ")) {
        return status;
    }

    std::cerr << "[pd dev] Port " << requested_port
              << " is unavailable; selecting an available port automatically."
              << std::endl;
    return server.listen(0, false, "127.0.0.1");
}

bool build_generated_app(
    const std::filesystem::path& project_root,
    const std::string& app_name,
    const std::filesystem::path& app_build_dir,
    const std::filesystem::path& executable) {
    const auto routes = pingcore::RouteCompiler::compile(project_root);
    if (!routes.success) {
        std::cerr << "[pd dev] Route compilation failed: " << routes.error_message << std::endl;
        return false;
    }

    const std::filesystem::path core_source{PINGCORE_SOURCE_DIR};
    const std::filesystem::path core_build{PINGCORE_BUILD_DIR};
    if (core_source.empty() || core_build.empty()) {
        std::cerr << "[pd dev] This pd binary does not have PingCore build paths configured." << std::endl;
        return false;
    }

    const std::string core_configure_command =
        "cmake -S " + quote_shell_argument(core_source.string()) +
        " -B " + quote_shell_argument(core_build.string());
    if (std::system(core_configure_command.c_str()) != 0) {
        std::cerr << "[pd dev] Could not configure the PingCore SDK build." << std::endl;
        return false;
    }

    const std::string app_configure_command =
        "cmake -S " + quote_shell_argument(project_root.string()) +
        " -B " + quote_shell_argument(app_build_dir.string()) +
        " -DCMAKE_PREFIX_PATH=" + quote_shell_argument(core_build.string());
    if (std::system(app_configure_command.c_str()) != 0) {
        std::cerr << "[pd dev] Could not configure the generated app." << std::endl;
        return false;
    }

    const std::string target = app_name + "_web_dev";
    const std::string build_command =
        "cmake --build " + quote_shell_argument(app_build_dir.string()) +
        " --target " + target;
    std::cout << "[pd dev] Building " << target << "..." << std::endl;
    if (std::system(build_command.c_str()) != 0) {
        std::cerr << "[pd dev] Build failed; keeping the last working page." << std::endl;
        return false;
    }
    if (!std::filesystem::exists(executable)) {
        std::cerr << "[pd dev] Build completed but executable was not found: "
                  << executable << std::endl;
        return false;
    }
    std::cout << "[pd dev] Page rebuilt successfully." << std::endl;
    return true;
}

bool render_generated_route(
    const std::filesystem::path& executable,
    const std::filesystem::path& temp_dir,
    const std::string& raw_path,
    std::string& html) {
    const auto query_start = raw_path.find('?');
    const std::string route_path = raw_path.substr(0, query_start);
    if (!pingdart::web::UrlSecurity::is_safe_uri_path(route_path)) {
        return false;
    }

    static std::atomic<uint64_t> render_id{0};
    const auto output_file = temp_dir /
        ("pingdart-app-route-" + std::to_string(render_id.fetch_add(1)) + ".html");
    const std::string render_command =
        quote_shell_argument(executable.string()) + " " +
        quote_shell_argument(route_path) + " > " + quote_shell_argument(output_file.string());
    const int result = std::system(render_command.c_str());
    if (result != 0) {
        std::error_code ec;
        std::filesystem::remove(output_file, ec);
        return false;
    }

    std::ifstream input(output_file, std::ios::binary);
    if (!input) {
        std::error_code ec;
        std::filesystem::remove(output_file, ec);
        return false;
    }
    std::string rendered(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());
    std::error_code ec;
    std::filesystem::remove(output_file, ec);
    if (rendered.empty()) return false;

    html = with_live_reload(std::move(rendered));
    return true;
}

int run_generated_app_dev_server(
    const std::filesystem::path& project_root,
    const std::string& app_name,
    uint16_t port) {
    const auto app_build_dir = project_root / "build";
#if defined(_WIN32)
    const auto executable = app_build_dir / (app_name + "_web_dev.exe");
#else
    const auto executable = app_build_dir / (app_name + "_web_dev");
#endif

    std::error_code ec;
    const auto temp_dir = std::filesystem::temp_directory_path(ec);
    if (ec) {
        std::cerr << "[pd dev] Could not locate the system temporary directory: "
                  << ec.message() << std::endl;
        return 1;
    }

    std::mutex build_mutex;
    auto build = [&] {
        std::lock_guard lock(build_mutex);
        return build_generated_app(project_root, app_name, app_build_dir, executable);
    };
    if (!build()) return 1;

    std::mutex revision_mutex;
    uint64_t revision = 1;
    pingcore::Server server;
    server.use([&](pingcore::Request& request, pingcore::Response& response, pingcore::Next next) {
        if (request.method() != "GET" || request.path() == "/__pingdart/revision") {
            next();
            return;
        }
        std::string html;
        {
            std::lock_guard lock(build_mutex);
            if (!render_generated_route(executable, temp_dir, request.path(), html)) {
                response.status(400).send("Could not render the requested route.");
                return;
            }
        }
        response.header("Content-Type", "text/html; charset=utf-8")
            .header("Cache-Control", "no-store")
            .send(html);
    });
    server.get("/__pingdart/revision", [&](const pingcore::Request&, pingcore::Response& response) {
        std::lock_guard lock(revision_mutex);
        response.header("Content-Type", "text/plain; charset=utf-8")
            .header("Cache-Control", "no-store")
            .send(std::to_string(revision));
    });

    const auto status = listen_with_port_fallback(server, port);
    if (!status.ok()) {
        std::cerr << "[pd dev] Could not start the local server: " << status.message() << std::endl;
        return 1;
    }

    std::cout << "[pd dev] Web dev server ready at http://127.0.0.1:"
              << server.port() << "/" << std::endl;
    std::cout << "[pd dev] Watching app/, components/, lib/, and PingCore headers. "
              << "Refresh is full-page, not state-preserving HMR. Press Ctrl+C to stop."
              << std::endl;

    const auto previous_sigint = std::signal(SIGINT, request_stop);
    const auto previous_sigterm = std::signal(SIGTERM, request_stop);
    const std::vector<std::filesystem::path> watched_roots{
        project_root / "CMakeLists.txt",
        project_root / "pingcore.yaml",
        project_root / "app",
        project_root / "components",
        project_root / "lib",
        project_root / "include",
        std::filesystem::path(PINGCORE_SOURCE_DIR) / "include" / "pingdart",
        std::filesystem::path(PINGCORE_SOURCE_DIR) / "include" / "pingcore"
    };
    auto known_files = take_snapshot(watched_roots);

    while (!stop_requested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        const auto changed_files = take_snapshot(watched_roots);
        if (changed_files == known_files) continue;

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        const auto stable_files = take_snapshot(watched_roots);
        if (stable_files != changed_files) {
            known_files = changed_files;
            continue;
        }

        if (build()) {
            const auto after_build = take_snapshot(watched_roots);
            if (after_build != stable_files) {
                known_files = stable_files;
                continue;
            }
            {
                std::lock_guard lock(revision_mutex);
                ++revision;
            }
            known_files = after_build;
        } else {
            known_files = stable_files;
        }
    }

    std::signal(SIGINT, previous_sigint);
    std::signal(SIGTERM, previous_sigterm);
    server.stop();
    std::cout << "[pd dev] Development server stopped." << std::endl;
    return 0;
}

} // namespace

int run_web_dev_server(
    const std::filesystem::path& project_root,
    const std::string& example_name,
    uint16_t port) {
    if (std::filesystem::exists(project_root / "app" / "page.cpp") &&
        std::filesystem::exists(project_root / "pingcore.yaml")) {
        return run_generated_app_dev_server(
            project_root, project_root.filename().string(), port);
    }

    const auto example_dir = project_root / "examples" / example_name;
    const auto build_dir = project_root / "build";
    const std::string target = "example_" + example_name;
#if defined(_WIN32)
    const auto executable = build_dir / "examples" / example_name / (target + ".exe");
#else
    const auto executable = build_dir / "examples" / example_name / target;
#endif

    if (!std::filesystem::exists(project_root / "CMakeLists.txt") ||
        !std::filesystem::exists(example_dir / "CMakeLists.txt")) {
        std::cerr << "[pd dev] Expected a configured PingDart project and examples/"
                  << example_name << "/CMakeLists.txt in " << project_root << std::endl;
        return 1;
    }
    if (!std::filesystem::exists(build_dir / "CMakeCache.txt")) {
        std::cerr << "[pd dev] Build directory is not configured: " << build_dir
                  << ". Configure the project with CMake first." << std::endl;
        return 1;
    }

    std::error_code ec;
    const auto temp_dir = std::filesystem::temp_directory_path(ec);
    if (ec) {
        std::cerr << "[pd dev] Could not locate the system temporary directory: "
                  << ec.message() << std::endl;
        return 1;
    }
    const auto unique_suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto html_file = temp_dir / ("pingdart-dev-" + std::to_string(unique_suffix) + ".html");

    std::string html;
    if (!build_and_render(build_dir, target, executable, html_file, html)) {
        std::filesystem::remove(html_file, ec);
        return 1;
    }

    std::mutex page_mutex;
    uint64_t revision = 1;
    pingcore::Server server;
    server.get("/", [&](const pingcore::Request&, pingcore::Response& response) {
        std::lock_guard lock(page_mutex);
        response.header("Content-Type", "text/html; charset=utf-8")
            .header("Cache-Control", "no-store")
            .send(html);
    });
    server.get("/__pingdart/revision", [&](const pingcore::Request&, pingcore::Response& response) {
        std::lock_guard lock(page_mutex);
        response.header("Content-Type", "text/plain; charset=utf-8")
            .header("Cache-Control", "no-store")
            .send(std::to_string(revision));
    });

    const auto status = listen_with_port_fallback(server, port);
    if (!status.ok()) {
        std::cerr << "[pd dev] Could not start the local server: " << status.message() << std::endl;
        std::filesystem::remove(html_file, ec);
        return 1;
    }

    std::cout << "[pd dev] Web dev server ready at http://127.0.0.1:"
              << server.port() << "/" << std::endl;
    std::cout << "[pd dev] Watching examples/" << example_name
              << " and PingDart headers. Press Ctrl+C to stop." << std::endl;

    const auto previous_sigint = std::signal(SIGINT, request_stop);
    const auto previous_sigterm = std::signal(SIGTERM, request_stop);

    std::vector<std::filesystem::path> watched_roots{
        project_root / "CMakeLists.txt",
        example_dir,
        project_root / "include" / "pingdart",
        project_root / "include" / "pingcore"
    };
    auto known_files = take_snapshot(watched_roots);

    while (!stop_requested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        const auto changed_files = take_snapshot(watched_roots);
        if (changed_files == known_files) continue;

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        const auto stable_files = take_snapshot(watched_roots);
        if (stable_files != changed_files) {
            known_files = changed_files;
            continue;
        }

        std::string updated_html;
        if (build_and_render(build_dir, target, executable, html_file, updated_html)) {
            const auto after_build = take_snapshot(watched_roots);
            if (after_build != stable_files) {
                known_files = stable_files;
                continue;
            }
            {
                std::lock_guard lock(page_mutex);
                html = std::move(updated_html);
                ++revision;
            }
            known_files = after_build;
        } else {
            known_files = stable_files;
        }
    }

    std::signal(SIGINT, previous_sigint);
    std::signal(SIGTERM, previous_sigterm);
    server.stop();
    std::filesystem::remove(html_file, ec);
    std::cout << "[pd dev] Development server stopped." << std::endl;
    return 0;
}
