// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

module;

#include <chrono>
#include <expected>
#include <optional>
#include <string>

export module walng.download;

export namespace walng {

struct download_response {
  unsigned response_code;
  std::optional<std::string> effective_url;
  std::optional<std::string> content_type;
  std::optional<std::string> content;
};

auto download(char const* url, std::optional<std::chrono::milliseconds> timeout = std::nullopt)
    -> std::expected<download_response, std::string>;

auto download(std::string const& url, std::optional<std::chrono::milliseconds> timeout = std::nullopt)
    -> std::expected<download_response, std::string> {
  return download(url.c_str(), timeout);
}

} // namespace walng
