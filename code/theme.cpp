// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "theme.h"

#include <array>
#include <format>
#include <span>

#include <yaml-cpp/yaml.h>

#include "colors.h"

namespace walng {
namespace {

static constexpr std::array kBase16Color{"base00", "base01", "base02", "base03", "base04", "base05", "base06", "base07",
    "base08", "base09", "base0A", "base0B", "base0C", "base0D", "base0E", "base0F"};

static constexpr std::array kBase24Color{"base00", "base01", "base02", "base03", "base04", "base05", "base06", "base07",
    "base08", "base09", "base0A", "base0B", "base0C", "base0D", "base0E", "base0F", "base10", "base11", "base12",
    "base13", "base14", "base15", "base16", "base17"};

} // namespace

inja::json loadBaseXXThemeFromYAMLFile(std::filesystem::path const& path) {
  auto const yamlRootNode = YAML::LoadFile(path.c_str());
  auto jsonRootNode = inja::json::object();

  for (auto const entry : {"name", "author", "variant"}) {
    jsonRootNode[entry] = yamlRootNode[entry].as<std::string>();
  }

  std::span<char const* const> paletteColors;
  auto const system = yamlRootNode["system"].as<std::string>();
  if (system == "base16") {
    paletteColors = std::span(kBase16Color);
  } else if (system == "base24") {
    paletteColors = std::span(kBase24Color);
  } else {
    throw std::runtime_error(std::format("unknown palette color system ('{}')", system));
  }

  auto const yamlPaletteNode = yamlRootNode["palette"];
  auto jsonPaletteNode = inja::json::object();
  for (auto const color : paletteColors) {
    auto const hexColorStr = yamlPaletteNode[color].as<std::string>();
    if (auto const value = parseColorFromHexStr(hexColorStr); !value) {
      throw std::runtime_error(std::format("invalid color value '{}' ('{}')", color, hexColorStr));
    }
    jsonPaletteNode[color] = hexColorStr;
  }
  jsonRootNode["palette"] = jsonPaletteNode;

  return jsonRootNode;
}

} // namespace walng
