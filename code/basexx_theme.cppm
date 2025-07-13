// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

module;

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

import walng.color;

export module walng.basexx_theme;

namespace walng {

struct basexx_theme {
  /// theme name
  std::string name;
  /// theme author
  std::string author;
  /// dark or light
  std::string variant;
  /// base16 or base24
  std::string system;
  /// palette
  std::vector<color> palette;
};

/// parse theme from yaml content
export [[nodiscard]] auto basexx_theme_parse_from_yaml_content(std::string const& content)
    -> std::expected<basexx_theme, std::string>;

/// parse theme from file with yaml content
export [[nodiscard]] auto basexx_theme_parse_from_yaml_file(std::filesystem::path const& path)
    -> std::expected<basexx_theme, std::string>;

} // namespace walng
