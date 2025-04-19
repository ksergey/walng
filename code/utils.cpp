// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "utils.h"

namespace walng {

std::filesystem::path const& homePath() {
  static std::filesystem::path homePath = [] {
    if (auto const result = ::secure_getenv("HOME"); result) {
      return std::filesystem::path(result);
    }
    throw std::runtime_error("$HOME environment not set");
  }();
  return homePath;
}

std::filesystem::path const& configPathBase() {
  static std::filesystem::path configPathBase = [] {
    if (auto const result = secure_getenv("XDG_CONFIG_HOME"); result) {
      return std::filesystem::path(result) / "walng";
    }
    return homePath() / ".config" / "walng";
  }();
  return configPathBase;
}

void expandTilda(std::filesystem::path& path) {
  if (auto const& str = path.native(); str.starts_with("~/")) {
    path = homePath() / std::string_view(str).substr(2);
  }
}

} // namespace walng
