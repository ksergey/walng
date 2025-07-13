// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <expected>
#include <filesystem>
#include <print>
#include <ranges>

#include <cxxopts.hpp>
#include <nlohmann/json.hpp>

import walng.basexx_theme;
import walng.color;
import walng.config;
import walng.download;
import walng.utils;
import walng.version;

int main(int argc, char* argv[]) {
  try {
    cxxopts::Options options("walng", "color template generator for base16 framework\n");

    // clang-format off
    options.add_options()
      ("config", "path to config file", cxxopts::value<std::string>(), "PATH")
      ("theme", "path or url to theme file", cxxopts::value<std::string>(), "PATH or URL")
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

    std::filesystem::path config_path;
    if (result.count("config")) {
      config_path = result["config"].as<std::string>();
    } else {
      auto default_config_path = walng::get_config_path();
      if (!default_config_path) {
        std::print(stderr, "failed to get default config file path ({})\n", default_config_path.error());
        return EXIT_FAILURE;
      }
      config_path = *default_config_path / "config.yaml";
    }

    auto config = walng::load_config_from_yaml_file(config_path);
    if (!config) {
      std::print(stderr, "failed to load config file '{}' ({})\n", config_path.c_str(), config.error());
      return EXIT_FAILURE;
    }
    std::print("CONFIG: {}\n", config_path.string());

    auto download_result = walng::download(
        "https://raw.githubusercontent.com/tinted-theming/schemes/refs/heads/spec-0.11/base16/black-metal-venom.yaml");
    if (!download_result) {
      std::print(stderr, "can't download theme ({})\n", download_result.error());
      return EXIT_FAILURE;
    }

    auto const& response = *download_result;
    if (response.response_code != 200) {
      std::print(stderr, "download theme error (response_code {})\n", response.response_code);
      return EXIT_FAILURE;
    }
    // if (response.content_type != "application/json") {
    //   std::print(stderr, "download theme error: content type \"{}\" instead of \"application/json\"\n",
    //       response.content_type.value_or("null"));
    //   return EXIT_FAILURE;
    // }

    if (!response.content) {
      std::print(stderr, "download theme error (no content)\n");
      return EXIT_FAILURE;
    }
    auto parse_result = walng::basexx_theme_parse_from_yaml_content(*response.content);
    if (!parse_result) {
      std::print(stderr, "theme parse error ({})\n", parse_result.error());
      return EXIT_FAILURE;
    }

    auto const& theme = *parse_result;
    std::print(stdout, "theme successful loaded\n");
    std::print(stdout, "  name: \"{}\"\n", theme.name);
    std::print(stdout, "  author: \"{}\"\n", theme.author);
    std::print(stdout, "  variant: \"{}\"\n", theme.variant);
    std::print(stdout, "  system: \"{}\"\n", theme.system);
    std::print(stdout, "  palette:\n");
    for (auto const& [index, color] : theme.palette | std::ranges::views::enumerate) {
      std::print(stdout, "    base{:02X}: \"#{}\"\n", index, color.as_hex_str().c_str());
    }

  } catch (std::exception const& e) {
    std::print(stderr, "Critical: {}\n", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
