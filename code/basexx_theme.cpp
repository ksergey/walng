// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

module;

#include <array>
#include <expected>
#include <filesystem>
#include <format>
#include <span>

#include <yaml-cpp/yaml.h>

import walng.color;
import walng.utils;

module walng.basexx_theme;

namespace walng {
namespace {

constexpr std::array base16_colors{"base00", "base01", "base02", "base03", "base04", "base05", "base06", "base07",
    "base08", "base09", "base0A", "base0B", "base0C", "base0D", "base0E", "base0F"};

constexpr std::array base24_colors{"base00", "base01", "base02", "base03", "base04", "base05", "base06", "base07",
    "base08", "base09", "base0A", "base0B", "base0C", "base0D", "base0E", "base0F", "base10", "base11", "base12",
    "base13", "base14", "base15", "base16", "base17"};

} // namespace

auto basexx_theme_parse_from_yaml(YAML::Node const& yaml) -> std::expected<basexx_theme, std::string> {
  basexx_theme result;

  result.name = yaml["name"].as<std::string>();
  result.author = yaml["author"].as<std::string>();
  result.variant = yaml["variant"].as<std::string>();
  if (result.variant != "dark" && result.variant != "light") {
    return std::unexpected(std::format("unknown theme variant value ({})", result.variant));
  }
  result.system = yaml["system"].as<std::string>();

  std::span<char const* const> colors;
  if (result.system == "base16") {
    colors = std::span(base16_colors);
    result.palette.reserve(16);
  } else if (result.system == "base24") {
    colors = std::span(base24_colors);
    result.palette.reserve(24);
  } else {
    return std::unexpected(std::format("unknown theme system value ({})", result.system));
  }

  auto const yaml_palette_node = yaml["palette"];
  for (auto const& name : colors) {
    auto const hex_color_str = yaml_palette_node[name].as<std::string>();
    if (auto rc = parse_color_from_hex_str(hex_color_str); rc) {
      result.palette.emplace_back(*rc);
    } else {
      return std::unexpected(std::format("can' parse color '{}', {}", hex_color_str, rc.error()));
    }
  }

  return {std::move(result)};
}

auto basexx_theme_parse_from_yaml_content(std::string const& content) -> std::expected<basexx_theme, std::string> {
  try {
    return basexx_theme_parse_from_yaml(YAML::Load(content));
  } catch (YAML::Exception const& e) {
    return std::unexpected(e.what());
  }
}

auto basexx_theme_parse_from_yaml_file(std::filesystem::path const& path) -> std::expected<basexx_theme, std::string> {
  try {
    std::filesystem::path expanded_path = path;
    expand_tilda(expanded_path);
    return basexx_theme_parse_from_yaml(YAML::LoadFile(expanded_path.string()));
  } catch (YAML::Exception const& e) {
    return std::unexpected(e.what());
  }
}

} // namespace walng
