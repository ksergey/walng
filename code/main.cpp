// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <filesystem>
#include <print>

#include <cxxopts.hpp>

#include "Template.h"
#include "Theme.h"

int main(int argc, char* argv[]) {
  try {
    cxxopts::Options options("walng", "template coloscheme generator");

    // clang-format off
    options.add_options()
      ("theme", "path to theme file", cxxopts::value<std::string>())
      ("template", "path to template file", cxxopts::value<std::string>())
      ("help", "print help and exit")
    ;
    // clang-format on

    auto const result = options.parse(argc, argv);
    if (result.count("help")) {
      std::print(stdout, "{}\n", options.help());
      return EXIT_FAILURE;
    }

    if (result.count("theme") == 0) {
      throw std::runtime_error("argument '--theme' is required");
    }
    std::filesystem::path const themePath = result["theme"].as<std::string>();

    if (result.count("template") == 0) {
      throw std::runtime_error("argument '--template' is required");
    }
    std::filesystem::path const templatePath = result["template"].as<std::string>();

    auto theme = walng::Theme::loadFromYAMLFile(themePath);
    std::print("json:\n{}\n", theme.dump(2));

    walng::Template::render(theme, templatePath);

  } catch (std::exception const& e) {
    std::print(stderr, "Error: {}\n", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
