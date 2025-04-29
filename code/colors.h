// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>

namespace walng {

/// Color in RGB
struct RGB {
  std::uint8_t r; ///< Red [0..255]
  std::uint8_t g; ///< Green [0..255]
  std::uint8_t b; ///< Blue [0..255]
};

struct Color {
  /// 0xRRGGBB
  std::uint32_t value = 0;

  constexpr RGB asRGB() const noexcept {
    // clang-format off
    return RGB{
      static_cast<std::uint8_t>((value & 0x00FF0000) >> 16),
      static_cast<std::uint8_t>((value & 0x0000FF00) >> 8),
      static_cast<std::uint8_t>((value & 0x000000FF))
    };
    // clang-format on
  }

  constexpr auto operator<=>(Color const&) const = default;
};

/// Parse color from stripped hex-string
/// i.e. @c 99aef1
/// @return std::nullopt on str is not valid color in hex
[[nodiscard]] constexpr std::optional<Color> parseColorFromStrippedHexStr(std::string_view str) noexcept {
  if (str.size() != 6) {
    return std::nullopt;
  }
  std::uint32_t value;
  auto const [ptr, ec] = std::from_chars(str.begin(), str.end(), value, 16);
  if (ec != std::errc()) {
    return std::nullopt;
  }
  return Color{value};
}

/// Parse color from hex-string
/// i.e. @c #99aef1
/// @return std::nullopt on str is not valid color in hex
[[nodiscard]] constexpr std::optional<Color> parseColorFromHexStr(std::string_view str) noexcept {
  if (!str.starts_with('#')) {
    return std::nullopt;
  }
  return parseColorFromStrippedHexStr(str.substr(1));
}

} // namespace walng
