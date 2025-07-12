// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

module;

#include <array>
#include <charconv>
#include <cstdint>
#include <expected>
#include <string_view>

export module walng.color;

namespace walng {

export struct color_hex_str {
  char value[7] = {'\0'};

  constexpr operator char const*() const noexcept {
    return value;
  }

  constexpr operator std::string_view() const noexcept {
    return value;
  }

  constexpr auto c_str() const noexcept -> char const* {
    return value;
  }

  constexpr auto string() const noexcept -> std::string_view {
    return value;
  }
};

export struct color_rgb {
  std::uint8_t r; ///< Red [0..255]
  std::uint8_t g; ///< Green [0..255]
  std::uint8_t b; ///< Blue [0..255]
};

export struct color {
  /// 0xRRGGBB
  std::uint32_t value = 0;

  constexpr auto as_rgb() const noexcept -> color_rgb {
    // clang-format off
    return color_rgb{
      static_cast<std::uint8_t>((value & 0x00FF0000) >> 16),
      static_cast<std::uint8_t>((value & 0x0000FF00) >> 8),
      static_cast<std::uint8_t>((value & 0x000000FF))
    };
    // clang-format on
  }

  constexpr auto as_hex_str() const noexcept -> color_hex_str {
    constexpr std::array chars = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    auto color = as_rgb();
    color_hex_str result;

    result.value[0] = chars[color.r >> 4];
    result.value[1] = chars[color.r & 0x0F];
    result.value[2] = chars[color.g >> 4];
    result.value[3] = chars[color.g & 0x0F];
    result.value[4] = chars[color.b >> 4];
    result.value[5] = chars[color.b & 0x0F];

    return result;
  }

  constexpr auto operator<=>(color const&) const = default;
};

/// Parse color from stripped hex-string
/// i.e. @c 99aef1
export [[nodiscard]] constexpr auto parse_color_from_stripped_hex_str(std::string_view str) noexcept
    -> std::expected<color, std::string_view> {
  if (str.size() != 6) {
    return std::unexpected("invalid color string (size)");
  }
  std::uint32_t value;
  auto const [ptr, ec] = std::from_chars(str.begin(), str.end(), value, 16);
  if (ec != std::errc()) {
    return std::unexpected("invalid color string (hex chars)");
  }
  return {color{value}};
}

/// Parse color from hex-string
/// i.e. @c #99aef1
export [[nodiscard]] constexpr auto parse_color_from_hex_str(std::string_view str) noexcept
    -> std::expected<color, std::string_view> {
  if (!str.starts_with('#')) {
    return std::unexpected("invalid color string (# not found)");
  }
  return parse_color_from_stripped_hex_str(str.substr(1));
}

} // namespace walng
