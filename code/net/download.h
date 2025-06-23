// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <chrono>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

namespace walng::net {

struct Response {
  long code = 0;
  std::string content;
  std::filesystem::path filename;
};

/// Download file at url
auto download(char const* url, std::optional<std::chrono::milliseconds> timeout = std::nullopt)
    -> std::expected<Response, std::error_code>;

/// \overload
inline auto download(std::string const& url, std::optional<std::chrono::milliseconds> timeout = std::nullopt)
    -> std::expected<Response, std::error_code> {
  return download(url.c_str(), timeout);
}

} // namespace walng::net
