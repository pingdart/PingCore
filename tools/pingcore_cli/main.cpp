#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#include "cli.h"
#include "prompt.h"
#include "sdk.h"

int main(int argc, char** argv) {
  std::vector<std::string_view> arguments;
  arguments.reserve(static_cast<std::size_t>(argc > 0 ? argc - 1 : 0));
  for (int index = 1; index < argc; ++index) {
    arguments.emplace_back(argv[index]);
  }
  try {
    const auto executable = pingcore::cli::ExecutablePath(argc > 0 ? argv[0] : "pingcore");
    const auto sdk = pingcore::cli::LocatePingCoreHome(executable);
    const auto prompter = pingcore::cli::prompt::MakeTerminalPrompter();
    return pingcore::cli::Run(arguments, std::filesystem::current_path(), sdk, std::cin, std::cout, std::cerr,
                             prompter.get());
  } catch (const std::exception& exception) {
    std::cerr << "pingcore: " << exception.what() << '\n';
    return 1;
  }
}
