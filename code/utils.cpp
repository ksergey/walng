// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "utils.h"

#include <random>

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
  static std::filesystem::path path = [] {
    if (auto const result = secure_getenv("XDG_CONFIG_HOME"); result) {
      return std::filesystem::path(result) / "walng";
    }
    return homePath() / ".config" / "walng";
  }();
  return path;
}

std::filesystem::path const& cacheBasePath() {
  static std::filesystem::path path = [] {
    if (auto const result = secure_getenv("XDG_CACHE_HOME"); result) {
      return std::filesystem::path(result) / "walng";
    }
    return homePath() / ".cache" / "walng";
  }();
  return path;
}

void expandTilda(std::filesystem::path& path) {
  if (auto const& str = path.native(); str.starts_with("~/")) {
    path = homePath() / std::string_view(str).substr(2);
  }
}

std::filesystem::path makeTemp() {
  static constexpr std::string_view kAllowedChars = "abcdefghijklmnaoqrstuvwxyz1234567890";

  std::random_device randomDevice;
  std::mt19937 gen(randomDevice());
  std::uniform_int_distribution<> dist(0, kAllowedChars.size() - 1);

  std::array<char, 16> tmpName;
  for (auto& ch : tmpName) {
    ch = kAllowedChars[dist(gen)];
  }

  return std::filesystem::temp_directory_path() / std::string_view(tmpName.data(), tmpName.size());
}

} // namespace walng
