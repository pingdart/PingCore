// src/pkg/manifest.cpp
#include <pingdart/pkg/manifest.h>
#include <fstream>
#include <sstream>

namespace pingdart::pkg {

namespace {

std::string_view trim_sv(std::string_view sv) {
    while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' || sv.front() == '\r')) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t' || sv.back() == '\r')) {
        sv.remove_suffix(1);
    }
    return sv;
}

std::string unquote(std::string_view sv) {
    sv = trim_sv(sv);
    if (sv.size() >= 2 && sv.front() == '"' && sv.back() == '"') {
        return std::string(sv.substr(1, sv.size() - 2));
    }
    if (sv.size() >= 2 && sv.front() == '\'' && sv.back() == '\'') {
        return std::string(sv.substr(1, sv.size() - 2));
    }
    return std::string(sv);
}

} // namespace

core::Result<PackageManifest> PackageManifest::parse(std::string_view toml_content) {
    PackageManifest manifest;
    std::istringstream stream{std::string(toml_content)};
    std::string line;
    std::string current_section = "";

    while (std::getline(stream, line)) {
        std::string_view trimmed = trim_sv(line);
        if (trimmed.empty() || trimmed.front() == '#') {
            continue; // Skip blank lines and comments
        }

        // Section headers: [section]
        if (trimmed.front() == '[' && trimmed.back() == ']') {
            current_section = std::string(trim_sv(trimmed.substr(1, trimmed.size() - 2)));
            continue;
        }

        size_t eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) {
            continue;
        }

        std::string key = std::string(trim_sv(trimmed.substr(0, eq_pos)));
        std::string_view val_str = trim_sv(trimmed.substr(eq_pos + 1));

        if (current_section == "package") {
            if (key == "name") {
                manifest.metadata_.name = unquote(val_str);
            } else if (key == "version") {
                auto v_res = SemVersion::parse(unquote(val_str));
                if (v_res.has_value()) {
                    manifest.metadata_.version = v_res.value();
                }
            } else if (key == "description") {
                manifest.metadata_.description = unquote(val_str);
            } else if (key == "license") {
                manifest.metadata_.license = unquote(val_str);
            } else if (key == "edition") {
                manifest.metadata_.edition = unquote(val_str);
            } else if (key == "repository") {
                manifest.metadata_.repository = unquote(val_str);
            }
        } else if (current_section == "platforms") {
            bool is_true = (val_str == "true");
            if (key == "android") manifest.platforms_.android = is_true;
            else if (key == "ios") manifest.platforms_.ios = is_true;
            else if (key == "windows") manifest.platforms_.windows = is_true;
            else if (key == "macos") manifest.platforms_.macos = is_true;
            else if (key == "linux") manifest.platforms_.linux = is_true;
            else if (key == "web") manifest.platforms_.web = is_true;
        } else if (current_section == "dependencies") {
            std::string req = unquote(val_str);
            auto c_res = VersionConstraint::parse(req);
            Dependency dep{
                .name = key,
                .constraint = c_res.has_value() ? c_res.value() : VersionConstraint{},
                .raw_requirement = req,
                .local_path = ""
            };
            manifest.dependencies_[key] = dep;
        } else if (current_section == "dev-dependencies" || current_section == "dev_dependencies") {
            std::string req = unquote(val_str);
            auto c_res = VersionConstraint::parse(req);
            Dependency dep{
                .name = key,
                .constraint = c_res.has_value() ? c_res.value() : VersionConstraint{},
                .raw_requirement = req,
                .local_path = ""
            };
            manifest.dev_dependencies_[key] = dep;
        }
    }

    if (manifest.metadata_.name.empty()) {
        return core::Status(core::StatusCode::InvalidArgument, "Manifest missing required [package].name");
    }

    return manifest;
}

core::Result<PackageManifest> PackageManifest::load_from_file(const std::filesystem::path& file_path) {
    std::ifstream in(file_path);
    if (!in.is_open()) {
        return core::Status(core::StatusCode::NotFound, "Manifest file not found: " + file_path.string());
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return parse(content);
}

std::string PackageManifest::to_toml_string() const {
    std::ostringstream ss;
    ss << "[package]\n";
    ss << "name = \"" << metadata_.name << "\"\n";
    ss << "version = \"" << metadata_.version.to_string() << "\"\n";
    if (!metadata_.description.empty()) {
        ss << "description = \"" << metadata_.description << "\"\n";
    }
    ss << "license = \"" << metadata_.license << "\"\n";
    ss << "edition = \"" << metadata_.edition << "\"\n";
    if (!metadata_.repository.empty()) {
        ss << "repository = \"" << metadata_.repository << "\"\n";
    }

    ss << "\n[dependencies]\n";
    for (const auto& [name, dep] : dependencies_) {
        ss << name << " = \"" << dep.raw_requirement << "\"\n";
    }

    if (!dev_dependencies_.empty()) {
        ss << "\n[dev-dependencies]\n";
        for (const auto& [name, dep] : dev_dependencies_) {
            ss << name << " = \"" << dep.raw_requirement << "\"\n";
        }
    }

    ss << "\n[platforms]\n";
    ss << "android = " << (platforms_.android ? "true" : "false") << "\n";
    ss << "ios = " << (platforms_.ios ? "true" : "false") << "\n";
    ss << "windows = " << (platforms_.windows ? "true" : "false") << "\n";
    ss << "macos = " << (platforms_.macos ? "true" : "false") << "\n";
    ss << "linux = " << (platforms_.linux ? "true" : "false") << "\n";
    ss << "web = " << (platforms_.web ? "true" : "false") << "\n";

    return ss.str();
}

core::Status PackageManifest::save_to_file(const std::filesystem::path& file_path) const {
    std::ofstream out(file_path);
    if (!out.is_open()) {
        return core::Status(core::StatusCode::Internal, "Failed to open file for writing: " + file_path.string());
    }
    out << to_toml_string();
    return core::Status{};
}

void PackageManifest::add_dependency(const std::string& name, const std::string& req_str) {
    auto c_res = VersionConstraint::parse(req_str);
    Dependency dep{
        .name = name,
        .constraint = c_res.has_value() ? c_res.value() : VersionConstraint{},
        .raw_requirement = req_str,
        .local_path = ""
    };
    dependencies_[name] = dep;
}

bool PackageManifest::remove_dependency(const std::string& name) {
    return dependencies_.erase(name) > 0;
}

} // namespace pingdart::pkg
