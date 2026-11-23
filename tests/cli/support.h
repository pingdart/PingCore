#pragma once

#include <catch2/catch_amalgamated.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "cli.h"
#include "platform.h"
#include "process_runner.h"
#include "project.h"
#include "sdk.h"
#include "template.h"

namespace pingcore::cli::test {

class TemporaryDirectory final {
public:
  TemporaryDirectory() {
    path_ = std::filesystem::temp_directory_path() /
            ("pingcore-cli-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(path_);
  }

  ~TemporaryDirectory() {
    std::error_code error;
    std::filesystem::remove_all(path_, error);
  }

  const std::filesystem::path& Path() const noexcept {
    return path_;
  }

private:
  std::filesystem::path path_;
};

// Replaces PATH for one scope, so that which tools a test finds does not depend on the machine running it.
class ScopedPath final {
public:
  explicit ScopedPath(const std::filesystem::path& directory)
      : previous_(pingcore::cli::ReadEnvironmentVariable("PATH").value_or("")) {
    pingcore::cli::SetProcessEnvironmentVariable("PATH", directory.string());
  }

  ~ScopedPath() {
    pingcore::cli::SetProcessEnvironmentVariable("PATH", previous_);
  }

  ScopedPath(const ScopedPath&) = delete;
  ScopedPath& operator=(const ScopedPath&) = delete;

private:
  std::string previous_;
};

struct Invocation {
  int result = 0;
  std::string output;
  std::string error;
};

inline Invocation Invoke(const std::filesystem::path& directory, std::initializer_list<std::string_view> arguments) {
  const std::vector<std::string_view> values(arguments);
  std::ostringstream output;
  std::ostringstream error;
  std::istringstream input("n\n");
  pingcore::cli::SdkLocation sdk;
  if (!values.empty() && values.front() == "create") {
    sdk = {PINGCORE_TEST_SOURCE_DIRECTORY, pingcore::cli::SdkLocationSource::Executable};
  }
  const int result = pingcore::cli::Run(values, directory, sdk, input, output, error);
  return {result, output.str(), error.str()};
}

inline std::string NormalizeLineEndings(std::string content) {
  std::erase(content, '\r');
  return content;
}

inline std::string ReadBinary(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  REQUIRE(stream);
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

inline std::string Read(const std::filesystem::path& path) {
  return NormalizeLineEndings(ReadBinary(path));
}

inline std::vector<std::string> ExpectedLibraryGraphArguments(const pingcore::cli::PlatformCommandContext& context) {
  std::vector<std::string> arguments{
      "-S",
      context.project_root.string(),
      "-B",
      (context.project_root / ".pingcore/build" /
       pingcore::cli::BuildHomeKey(context.pingcore_home) / "library-graph").string(),
      "-DPINGCORE_LIBRARY_GRAPH_ONLY=ON",
      "-DPINGCORE_LIBRARY_GRAPH_OUTPUT=" + (context.project_root / ".pingcore/generated/libraries.json").string(),
      "-DPINGCORE_HOME=" + context.pingcore_home.string(),
  };
  if (!pingcore::cli::ReadEnvironmentVariable("CMAKE_GENERATOR") && pingcore::cli::FindExecutable("ninja")) {
    arguments.insert(arguments.begin(), {"-G", "Ninja"});
  }
  return arguments;
}

#if !defined(_WIN32)
inline bool IsExecutable(const std::filesystem::path& path) {
  const std::filesystem::perms permissions = std::filesystem::status(path).permissions();
  return (permissions & (std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec |
                         std::filesystem::perms::others_exec)) != std::filesystem::perms::none;
}
#endif

} // namespace pingcore::cli::test
