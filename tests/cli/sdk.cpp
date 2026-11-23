#include "support.h"

using namespace pingcore::cli::test;

TEST_CASE("PingCoreCliBuildSelectionRequiresAnExplicitSdkOverrideForSource") {
  using namespace pingcore::cli;
  TemporaryDirectory temporary;
  const auto source = std::filesystem::path(PINGCORE_TEST_SOURCE_DIRECTORY);
  REQUIRE_THROWS_WITH(ResolveBuildHome(source, std::nullopt, temporary.Path()),
                      Catch::Matchers::ContainsSubstring("--sdk"));
  REQUIRE_THROWS_AS(ResolveBuildHome({}, std::nullopt, temporary.Path()), std::runtime_error);
  const auto selected = ResolveBuildHome(temporary.Path() / "missing-sdk", source, temporary.Path());
  REQUIRE(std::filesystem::equivalent(selected, source));
  REQUIRE(BuildHomeKey(selected) == BuildHomeKey(source / "."));
  REQUIRE(BuildHomeKey(selected) != BuildHomeKey(temporary.Path()));
}

TEST_CASE("PingCoreCliValidatesSdkOptionsBeforeLocatingSdk") {
  TemporaryDirectory temporary;
  REQUIRE(Invoke(temporary.Path(), {"--help"}).output.find("pingcore sdk update") != std::string::npos);
  const auto help = Invoke(temporary.Path(), {"sdk", "--help"});
  REQUIRE(help.result == 0);
  REQUIRE(help.output.find("pingcore sdk list") != std::string::npos);
  REQUIRE(help.output.find("pingcore sdk use <version>") != std::string::npos);
  for (const auto arguments : {std::initializer_list<std::string_view>{"sdk", "use"},
                               {"sdk", "use", ""}, {"sdk", "use", "../0.4.0"}, {"sdk", "use", "0.4..0"},
                               {"sdk", "use", "0.4.0", "--check"}, {"sdk", "list", "--yes"},
                               {"sdk", "update", "--check", "--check"}, {"sdk", "update", "--yes", "--yes"},
                               {"sdk", "update", "--version", "0.4.0"}, {"update", "--check"}}) {
    REQUIRE(Invoke(temporary.Path(), arguments).result == 2);
  }
  const auto missing = Invoke(temporary.Path(), {"sdk", "update", "--check"});
  REQUIRE(missing.result == 1);
  REQUIRE(missing.error.find("require an installed SDK") != std::string::npos);
}

TEST_CASE("PingCoreCliSdkManagementRequiresTheSelectedInstallationsExecutable") {
  using namespace pingcore::cli;
  TemporaryDirectory temporary;
  const auto root = temporary.Path();
  for (const auto path : {"bin", "include/pingcore", "lib/cmake/PingCore", "share/pingcore/tools",
                          "share/pingcore/resources/pingcore", "share/pingcore/skills/pingcore-app-development"}) {
    std::filesystem::create_directories(root / path);
  }
#if defined(_WIN32)
  const auto executable = root / "bin/pingcore.exe";
#else
  const auto executable = root / "bin/pingcore";
#endif
  for (const auto& path : {executable, root / "include/pingcore/pingcore.h",
                          root / "lib/cmake/PingCore/PingCoreConfig.cmake",
                          root / "share/pingcore/resources/pingcore/resources.bin"}) {
    std::ofstream(path) << "fixture";
  }
  const SdkLocation sdk{root, SdkLocationSource::Environment};
  REQUIRE_NOTHROW(ValidateManagedSdk(sdk, executable));
  REQUIRE_THROWS_WITH(ValidateManagedSdk(sdk, ExecutablePath({})),
                      Catch::Matchers::ContainsSubstring("different installations"));
  REQUIRE_THROWS_WITH(ValidateManagedSdk({PINGCORE_TEST_SOURCE_DIRECTORY, SdkLocationSource::Environment}, executable),
                      Catch::Matchers::ContainsSubstring("source workflow"));
}

TEST_CASE("PingCoreCliDoctorAcceptsAReorganizedSourceDirectory") {
  TemporaryDirectory temporary;
  REQUIRE(Invoke(temporary.Path(), {"create", "app", "sample", "--platform", "windows"}).result == 0);
  const std::filesystem::path project = temporary.Path() / "sample";
  std::filesystem::rename(project / "src/app.cpp", project / "src/application.cpp");

  const Invocation invocation = Invoke(project, {"doctor", "windows"});

  REQUIRE(invocation.output.find("missing src") == std::string::npos);
}

TEST_CASE("PingCoreCliDoctorReportsTheResolvedSdk") {
  TemporaryDirectory temporary;
  for (const auto directory : {"include/pingcore", "lib/cmake/PingCore", "share/pingcore/tools",
                               "share/pingcore/resources/pingcore", "share/pingcore/skills/pingcore-app-development"}) {
    std::filesystem::create_directories(temporary.Path() / directory);
  }
  for (const auto file : {"include/pingcore/pingcore.h", "lib/cmake/PingCore/PingCoreConfig.cmake",
                          "share/pingcore/resources/pingcore/resources.bin"}) {
    std::ofstream(temporary.Path() / file) << "fixture";
  }
  const std::optional<std::filesystem::path> cmake = pingcore::cli::FindExecutable("cmake");
  REQUIRE(cmake);
  const std::vector<std::string_view> arguments{"doctor"};
  std::ostringstream output;
  std::ostringstream error;
  std::istringstream input;

  const int result = pingcore::cli::Run(
      arguments,
      temporary.Path(),
      {temporary.Path(), pingcore::cli::SdkLocationSource::Environment},
      input,
      output,
      error
  );

  REQUIRE(result == 0);
  REQUIRE(output.str().find("[ok] PINGCORE_HOME (environment): " + temporary.Path().string()) != std::string::npos);
  REQUIRE(output.str().find("[ok] cmake: " + cmake->string()) != std::string::npos);
  REQUIRE(error.str().empty());
}
