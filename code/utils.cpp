// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "utils.h"

#include <random>

namespace walng {

std::expected<std::filesystem::path, std::system_error> getHomePath() {
  if (auto const result = ::secure_getenv("HOME"); result) {
    return std::filesystem::path(result);
  }
  return std::unexpected(std::system_error(ENOENT, std::system_category(), "HOME env variable not set"));
}

std::expected<std::filesystem::path, std::system_error> getConfigPath() {
  if (auto const result = ::secure_getenv("XDG_CONFIG_HOME"); result) {
    return std::filesystem::path(result) / "walng";
  }
  return getHomePath().transform([](std::filesystem::path const& path) {
    return path / ".config" / "walng";
  });
}

std::expected<std::filesystem::path, std::system_error> getCachePath() {
  if (auto const result = ::secure_getenv("XDG_CACHE_HOME"); result) {
    return std::filesystem::path(result) / "walng";
  }
  return getHomePath().transform([](std::filesystem::path const& path) {
    return path / ".cache" / "walng";
  });
}

std::expected<std::filesystem::path, std::system_error> makeTempFilePath() {
  static constexpr std::string_view kAllowedChars = "abcdefghijklmnaoqrstuvwxyz1234567890";

  std::random_device randomDevice;
  std::mt19937 gen(randomDevice());
  std::uniform_int_distribution<> dist(0, kAllowedChars.size() - 1);

  std::array<char, 16> tmpName;
  for (auto& ch : tmpName) {
    ch = kAllowedChars[dist(gen)];
  }

  std::error_code ec;
  auto tempDirPath = std::filesystem::temp_directory_path(ec);
  if (ec) {
    return std::unexpected(std::system_error(ec, "can't obtain temp directory path"));
  }
  return tempDirPath / std::string_view(tmpName.data(), tmpName.size());
}

std::expected<void, std::system_error> expandTilda(std::filesystem::path& path) {
  if (auto const& str = path.native(); str.starts_with("~/")) {
    auto homePath = getHomePath();
    if (!homePath.has_value()) {
      return std::unexpected(homePath.error());
    }
    path = homePath.value() / std::string_view(str).substr(2);
  }
  return {};
}

} // namespace walng
