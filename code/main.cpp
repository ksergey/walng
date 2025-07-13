// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <print>
#include <ranges>

#include <cxxopts.hpp>
#include <inja/inja.hpp>

import walng.basexx_theme;
import walng.color;
import walng.config;
import walng.download;
import walng.utils;
import walng.version;

auto get_inja_env() -> inja::Environment {
  inja::Environment result;

  result.set_trim_blocks(true);
  result.set_lstrip_blocks(true);

  result.add_callback("hex", 1, [](inja::Arguments const& args) -> std::string {
    auto color_result = walng::parse_color_from_hex_str(args.at(0)->get<std::string>());
    if (!color_result) {
      throw std::runtime_error(std::string(color_result.error()));
    }
    return std::string(color_result->as_hex_str().string());
  });

  result.add_callback("rgb", 1, [](inja::Arguments const& args) -> std::string {
    auto color_result = walng::parse_color_from_hex_str(args.at(0)->get<std::string>());
    if (!color_result) {
      throw std::runtime_error(std::string(color_result.error()));
    }
    auto rgb = color_result->as_rgb();
    return std::format("{}, {}, {}", rgb.r, rgb.g, rgb.b);
  });

  return result;
}

auto basexx_theme_to_json(walng::basexx_theme const& theme) -> inja::json {
  auto json = inja::json::object();

  json["name"] = theme.name;
  json["author"] = theme.author;
  json["variant"] = theme.variant;
  json["system"] = theme.system;

  auto json_palette = inja::json::object();

  char color_name[7];
  for (auto const& [index, color] : theme.palette | std::ranges::views::enumerate) {
    std::format_to_n(color_name, sizeof(color_name), "base{:02X}", index);
    json_palette[color_name] = std::string("#").append(color.as_hex_str().string());
  }

  json["palette"] = json_palette;

  return json;
}

auto execute_hook(std::string const& shell_exec_cmd, std::string const& hook_cmd) -> std::expected<void, std::string> {
  auto const found = shell_exec_cmd.find("{}");
  if (found == shell_exec_cmd.npos) {
    return std::unexpected("shell command without placeholder");
  }

  auto const command_to_execute =
      std::string().append(shell_exec_cmd, 0, found).append(hook_cmd).append(shell_exec_cmd, found + 2);
  if (auto const rc = ::system(command_to_execute.c_str()); rc == -1) {
    return std::unexpected(std::strerror(errno));
  }

  return {};
}

auto process(walng::config const& config, walng::basexx_theme const& theme) -> std::expected<void, std::string> {
  try {
    inja::Environment env = get_inja_env();
    inja::json const json = basexx_theme_to_json(theme);

    auto temp_file_path_result = walng::create_temporary_file_path();
    if (!temp_file_path_result) {
      return std::unexpected(std::format("can't create temporary file path ({})", temp_file_path_result.error()));
    }

    for (auto const& item : config.items) {
      std::print(stdout, "processing '{}'\n", item.name);

      // generate
      env.write(item.template_path, json, temp_file_path_result.value());

      // remove target file if exists
      if (std::filesystem::exists(item.target_path)) {
        std::filesystem::remove(item.target_path);
      }

      // copy file to target path
      std::filesystem::copy_file(temp_file_path_result.value(), item.target_path);

      // execute hook if exists
      if (!item.hook_cmd.empty()) {
        if (auto result = execute_hook(config.shell_exec_cmd, item.hook_cmd); !result) {
          std::print(stderr, "failed to execute hook ({})\n", result.error());
        }
      }
    }
  } catch (std::exception const& e) {
    return std::unexpected(e.what());
  }

  return {};
}

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
    std::print("CONFIG: {}\n", config_path.string());

    auto config_load_result = walng::load_config_from_yaml_file(config_path);
    if (!config_load_result) {
      std::print(stderr, "failed to load config file '{}' ({})\n", config_path.c_str(), config_load_result.error());
      return EXIT_FAILURE;
    }

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
    auto theme_parse_result = walng::basexx_theme_parse_from_yaml_content(*response.content);
    if (!theme_parse_result) {
      std::print(stderr, "theme parse error ({})\n", theme_parse_result.error());
      return EXIT_FAILURE;
    }

    auto const& theme = *theme_parse_result;
    std::print(stdout, "theme successful loaded\n");
    std::print(stdout, "  name: \"{}\"\n", theme.name);
    std::print(stdout, "  author: \"{}\"\n", theme.author);
    std::print(stdout, "  variant: \"{}\"\n", theme.variant);
    std::print(stdout, "  system: \"{}\"\n", theme.system);
    std::print(stdout, "  palette:\n");
    for (auto const& [index, color] : theme.palette | std::ranges::views::enumerate) {
      std::print(stdout, "    base{:02X}: \"#{}\"\n", index, color.as_hex_str().c_str());
    }

    // if (auto result = process(config_load_result.value(), theme_parse_result.value()); !result) {
    //   std::print(stderr, "failed to process ({})\n", result.error());
    // }

  } catch (std::exception const& e) {
    std::print(stderr, "Critical: {}\n", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
