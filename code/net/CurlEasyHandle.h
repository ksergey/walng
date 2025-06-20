// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <expected>
#include <utility>

#include <curl/curl.h>

#include "error.h"

namespace walng::net {

class CurlEasyHandle {
private:
  CURL* handle_ = nullptr;

public:
  CurlEasyHandle(CurlEasyHandle const&) = delete;
  CurlEasyHandle& operator=(CurlEasyHandle const&) = delete;

  CurlEasyHandle() = default;

  CurlEasyHandle(CurlEasyHandle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  ~CurlEasyHandle() noexcept {
    if (handle_) {
      ::curl_easy_cleanup(handle_);
    }
  }

  CurlEasyHandle& operator=(CurlEasyHandle&& other) noexcept {
    if (this != &other) {
      this->~CurlEasyHandle();
      new (this) CurlEasyHandle(std::move(other));
    }
    return *this;
  }

  explicit CurlEasyHandle(CURL* handle) noexcept : handle_(handle) {}

  /// Create and init CurlEasyHandle
  static std::expected<CurlEasyHandle, std::error_code> create() noexcept {
    auto handle = ::curl_easy_init();
    if (!handle) [[unlikely]] {
      return std::unexpected(make_error_code(CurlInitError::EasyInit));
    }
    return {CurlEasyHandle(handle)};
  }

  /// Return true on handle valid
  [[nodiscard]] operator bool() const noexcept {
    return handle_ != nullptr;
  }

  /// Wrapper around @c curl_easy_reset
  void reset() noexcept {
    ::curl_easy_reset(handle_);
  }

  /// Wrapper around @c curl_easy_setopt
  template <typename T>
  std::expected<void, std::error_code> setOption(CURLoption opt, T&& value) noexcept {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
      return this->setOptionImpl(opt, value.c_str());
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>) {
      return this->setOptionImpl(opt, std::string(value).c_str());
    } else {
      return this->setOptionImpl(opt, std::forward<T>(value));
    }
  }

  /// Wrapper around @c curl_easy_getinfo
  template <typename T>
  std::expected<T, std::error_code> info(CURLINFO info) const noexcept {
    if constexpr (std::is_same_v<T, std::string>) {
      return this->infoImpl<char const*>(info).transform([](char const* value) {
        return std::string(value);
      });
    } else if constexpr (std::is_same_v<T, std::string_view>) {
      return this->infoImpl<char const*>(info).transform([](char const* value) {
        return std::string_view(value);
      });
    } else {
      return this->infoImpl<T>(info);
    }
  }

  /// Wrapper around @c curl_easy_perform
  std::expected<void, std::error_code> perform() noexcept {
    if (auto const rc = ::curl_easy_perform(handle_); rc != CURLE_OK) [[unlikely]] {
      return std::unexpected(make_error_code(rc));
    }
    return {};
  }

private:
  template <typename T>
  inline std::expected<void, std::error_code> setOptionImpl(CURLoption opt, T&& value) noexcept {
    if (auto const rc = ::curl_easy_setopt(handle_, opt, std::forward<T>(value)); rc != CURLE_OK) [[unlikely]] {
      return std::unexpected(make_error_code(rc));
    }
    return {};
  }

  template <typename T>
  inline std::expected<T, std::error_code> infoImpl(CURLINFO info) const noexcept {
    T result;
    if (auto const rc = ::curl_easy_getinfo(handle_, info, &result); rc != CURLE_OK) [[unlikely]] {
      return std::unexpected(make_error_code(rc));
    }
    return {result};
  }
};

} // namespace walng::net
