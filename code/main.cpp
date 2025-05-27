// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <filesystem>
#include <print>

#include <cxxopts.hpp>

#include "generate.h"
#include "utils.h"
#include "version.h"

#include "curl_test.h"

int main(int argc, char* argv[]) {
  curl_test();
  return 0;

  try {
    cxxopts::Options options("walng", "color template generator for base16 framework\n");

    // clang-format off
    options.add_options()
      ("config", "path to config file", cxxopts::value<std::string>(), "PATH")
      ("theme", "path to theme file", cxxopts::value<std::string>(), "PATH")
      ("help", "prints the help and exit")
      ("version", "prints the version and exit")
    ;
    // clang-format on

    auto const result = options.parse(argc, argv);

    if (result.count("help")) {
      std::print(stdout, "{}\n", options.help());
      return EXIT_FAILURE;
    }

    if (result.count("version")) {
      std::print(stdout, "walng {}\n", walng::version);
      return EXIT_FAILURE;
    }

    std::filesystem::path const configPath = [&] {
      if (result.count("config")) {
        return std::filesystem::path(result["config"].as<std::string>());
      }
      return walng::configPathBase() / "config.yaml";
    }();

    if (result.count("theme") == 0) {
      throw std::runtime_error("argument '--theme' is required");
    }
    std::filesystem::path const themePath = result["theme"].as<std::string>();

    walng::generate(configPath, themePath);

  } catch (std::exception const& e) {
    std::print(stderr, "Error: {}\n", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
