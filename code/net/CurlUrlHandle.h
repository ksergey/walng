// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <expected>
#include <string>
#include <utility>

#include <curl/curl.h>

#include "error.h"

namespace walng::net {

class CurlUrlHandle {
private:
  CURLU* handle_ = nullptr;

public:
  CurlUrlHandle(CurlUrlHandle const&) = delete;
  CurlUrlHandle& operator=(CurlUrlHandle const&) = delete;

  explicit CurlUrlHandle(CURLU* handle) noexcept : handle_(handle) {}

  CurlUrlHandle() = default;

  ~CurlUrlHandle() noexcept {
    if (handle_) {
      ::curl_url_cleanup(handle_);
    }
  }

  CurlUrlHandle(CurlUrlHandle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  CurlUrlHandle& operator=(CurlUrlHandle&& other) noexcept {
    if (this != &other) {
      this->~CurlUrlHandle();
      new (this) CurlUrlHandle(std::move(other));
    }
    return *this;
  }

  /// Create and init CurlUrlHandle
  static std::expected<CurlUrlHandle, std::error_code> create() noexcept {
    auto handle = ::curl_url();
    if (!handle) [[unlikely]] {
      return std::unexpected(make_error_code(CurlInitError::UrlInit));
    }
    return {CurlUrlHandle(handle)};
  }

  /// Return true on handle valid
  [[nodiscard]] operator bool() const noexcept {
    return handle_ != nullptr;
  }

  /// Wrapper around @c curl_url_get
  std::expected<std::string, std::error_code> part(CURLUPart part) const {
    char* value = nullptr;
    if (auto const rc = ::curl_url_get(handle_, part, &value, 0); rc != CURLUE_OK) [[unlikely]] {
      return std::unexpected(make_error_code(rc));
    }
    std::string result(value);
    ::curl_free(value);
    return {result};
  }

  /// Wrapper around @c curl_url_set
  template <typename T>
  std::expected<void, std::error_code> setPart(CURLUPart part, T&& value) {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
      return this->setPartImpl(part, value.c_str());
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>) {
      return this->setPartImpl(part, std::string(value).c_str());
    } else {
      return this->setPartImpl(part, value);
    }
  }

private:
  std::expected<void, std::error_code> setPartImpl(CURLUPart part, char const* value) noexcept {
    if (auto const rc = ::curl_url_set(handle_, part, value, 0); rc != CURLUE_OK) [[unlikely]] {
      return std::unexpected(make_error_code(rc));
    }
    return {};
  }
};

} // namespace walng::net
