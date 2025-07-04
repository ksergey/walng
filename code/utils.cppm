// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

module;

#include <array>
#include <expected>
#include <filesystem>
#include <random>
#include <string_view>

export module walng.utils;

namespace walng {

auto get_home_path() -> std::expected<std::filesystem::path, std::string> {
  if (auto const result = ::secure_getenv("HOME"); result) {
    return std::filesystem::path(result);
  }
  return std::unexpected("environment variable $HOME not exists");
}

export auto get_config_path() -> std::expected<std::filesystem::path, std::string> {
  if (auto const result = ::secure_getenv("XDG_CONFIG_HOME"); result) {
    return std::filesystem::path(result) / "walng";
  }
  return get_home_path().transform([](std::filesystem::path const& path) {
    return path / ".config" / "walng";
  });
}

export auto get_cache_path() -> std::expected<std::filesystem::path, std::string> {
  if (auto const result = ::secure_getenv("XDG_CACHE_HOME"); result) {
    return std::filesystem::path(result) / "walng";
  }
  return get_home_path().transform([](std::filesystem::path const& path) {
    return path / ".cache" / "walng";
  });
}

export auto create_temporary_file_path() -> std::expected<std::filesystem::path, std::string> {
  static constexpr std::string_view allowed_chars = "abcdefghijklmnaoqrstuvwxyz1234567890";

  std::random_device device;
  std::mt19937 gen(device());
  std::uniform_int_distribution<> dist(0, allowed_chars.size() - 1);

  std::array<char, 16> temp_name;
  for (auto& ch : temp_name) {
    ch = allowed_chars[dist(gen)];
  }

  std::error_code ec;
  auto temp_directory_path = std::filesystem::temp_directory_path(ec);
  if (ec) {
    return std::unexpected(ec.message());
  }
  return temp_directory_path / std::string_view(temp_name.data(), temp_name.size());
}

export auto expand_tilda(std::filesystem::path& path) -> std::expected<void, std::string> {
  if (auto const& str = path.native(); str.starts_with("~/")) {
    auto home_path = get_home_path();
    if (!home_path.has_value()) {
      return std::unexpected(home_path.error());
    }
    path = home_path.value() / std::string_view(str).substr(2);
  }
  return {};
}

} // namespace walng
