// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "download.h"

#include <print>

#include "CurlEasyHandle.h"
#include "CurlUrlHandle.h"

namespace walng::net {
namespace {

size_t curlWriteFn(char const* data, size_t size, size_t nmemb, void* userdata) {
  auto const response = static_cast<Response*>(userdata);
  auto const chunkSize = size * nmemb;
  response->content.append(data, chunkSize);
  return chunkSize;
}

std::expected<std::filesystem::path, std::error_code> extractFilename(char const* url) {
  CurlUrlHandle handle;
  if (auto rc = CurlUrlHandle::create(); rc) {
    handle = std::move(*rc);
  } else {
    return std::unexpected(rc.error());
  }

  if (auto rc = handle.setPart(CURLUPART_URL, url); !rc) {
    return std::unexpected(rc.error());
  }

  std::filesystem::path path;
  if (auto rc = handle.part(CURLUPART_PATH); rc) {
    path = std::move(*rc);
  } else {
    return std::unexpected(rc.error());
  }

  return {path.filename()};
}

} // namespace

auto download(char const* url, std::optional<std::chrono::milliseconds> timeout)
    -> std::expected<Response, std::error_code> {
  CurlEasyHandle handle;
  if (auto rc = CurlEasyHandle::create(); rc) {
    handle = std::move(*rc);
  } else {
    return std::unexpected(rc.error());
  }

  Response response;

  if (auto rc = handle.setOption(CURLOPT_WRITEDATA, &response); !rc) {
    return std::unexpected(rc.error());
  }
  if (auto rc = handle.setOption(CURLOPT_WRITEFUNCTION, &curlWriteFn); !rc) {
    return std::unexpected(rc.error());
  }
  if (auto rc = handle.setOption(CURLOPT_FOLLOWLOCATION, 1L); !rc) {
    return std::unexpected(rc.error());
  }
  if (auto rc = handle.setOption(CURLOPT_URL, url); !rc) {
    return std::unexpected(rc.error());
  }
  if (timeout) {
    if (auto rc = handle.setOption(CURLOPT_TIMEOUT_MS, static_cast<long>(timeout->count())); !rc) {
      return std::unexpected(rc.error());
    }
  }

  if (auto rc = handle.perform(); !rc) {
    return std::unexpected(rc.error());
  }

  if (auto rc = handle.info<long>(CURLINFO_RESPONSE_CODE); rc) {
    response.code = *rc;
  } else {
    std::print(stderr, "CURLINFO_RESPONSE_CODE falied: {}\n", rc.error().message());
  }

  if (auto rc = handle.info<char const*>(CURLINFO_EFFECTIVE_URL); rc) {
    char const* effectiveUrl = *rc;
    if (auto rc = extractFilename(effectiveUrl); rc) {
      response.filename = *rc;
    } else {
      std::print(stderr, "can't extract downloaded file filename: {}\n", rc.error().message());
    }
  } else {
    std::print(stderr, "CURLINFO_EFFECTIVE_URL falied: {}\n", rc.error().message());
  }

  return {std::move(response)};
}

} // namespace walng::net
