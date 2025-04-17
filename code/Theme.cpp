// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "Theme.h"

#include <cctype>
#include <format>
#include <stdexcept>
#include <string_view>

#include <yaml-cpp/yaml.h>

namespace walng {
namespace {

[[nodiscard]] std::string_view getStrOrThrow(YAML::Node const& parent, std::string_view name) {
  auto const node = parent[name];
  if (!node) {
    throw std::runtime_error(std::format("node '{}' not found", name));
  }
  return node.as<std::string_view>();
}

[[nodiscard]] std::size_t getPaletteColorsCount(std::string_view system) noexcept {
  if (system == "base16") {
    return 16;
  }
  if (system == "base24") {
    return 24;
  }
  return 0;
}

[[nodiscard]] constexpr bool isValidHexColor(std::string_view str) noexcept {
  return str.size() == 7 && str[0] == '#' && std::isxdigit(str[1]) && std::isxdigit(str[2]) && std::isxdigit(str[3]) &&
         std::isxdigit(str[4]) && std::isxdigit(str[5]) && std::isxdigit(str[6]);
}

} // namespace

inja::json Theme::loadFromYAMLFile(std::filesystem::path const& path) {
  using namespace std::string_view_literals;

  auto const rootNode = YAML::LoadFile(path.c_str());

  inja::json result;
  // store filename
  result["file"sv] = absolute(path).c_str();

  // copy common fields
  for (auto const name : {"system"sv, "name"sv, "author"sv, "variant"sv}) {
    result[name] = getStrOrThrow(rootNode, name);
  }

  auto const paletteColors = getPaletteColorsCount(getStrOrThrow(rootNode, "system"sv));
  auto const paletteNode = rootNode["palette"];
  auto palette = inja::json::object();

  // copy baseXX
  for (std::size_t colorNo = 0; colorNo < paletteColors; ++colorNo) {
    auto const colorName = std::format("base{:02X}", colorNo);
    auto const colorHex = getStrOrThrow(paletteNode, colorName);
    if (!isValidHexColor(colorHex)) {
      throw std::runtime_error(std::format("invalid color hex-value ('{}')", colorHex));
    }
    result[colorName] = colorHex;
    palette[colorName] = colorHex;
  }

  result["palette"] = palette;

  return result;
}

} // namespace walng
