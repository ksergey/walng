// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <utility>

#include <curl/curl.h>

namespace walng {

/// Wrapper for curl easy handle
class CurlEasy {
private:
  CURL* handle_ = nullptr;

public:
  CurlEasy(CurlEasy const&) = delete;
  CurlEasy& operator=(CurlEasy const&) = delete;

  CurlEasy(CurlEasy&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  CurlEasy& operator=(CurlEasy&& other) noexcept {
    if (this != &other) {
      this->~CurlEasy();
      new (this) CurlEasy(std::move(other));
    }
    return *this;
  }

  CurlEasy() {
    handle_ = ::curl_easy_init();
    if (handle_ == nullptr) {
      throw std::runtime_error("failed to init curl multi handle");
    }
  }

  ~CurlEasy() noexcept {
    if (handle_) {
      ::curl_easy_cleanup(handle_);
    }
  }

  /// Return true on handle initialized
  [[nodiscard]] explicit operator bool() const noexcept {
    return handle_ != nullptr;
  }

  /// Return native handle
  [[nodiscard]] operator CURL*() const noexcept {
    return handle_;
  }
};

/// Wrapper for curl multi handle
class CurlMulti {
private:
  CURLM* handle_ = nullptr;

public:
  CurlMulti(CurlMulti const&) = delete;
  CurlMulti& operator=(CurlMulti const&) = delete;

  CurlMulti(CurlMulti&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  CurlMulti& operator=(CurlMulti&& other) noexcept {
    if (this != &other) {
      this->~CurlMulti();
      new (this) CurlMulti(std::move(other));
    }
    return *this;
  }

  CurlMulti() {
    handle_ = ::curl_multi_init();
    if (handle_ == nullptr) {
      throw std::runtime_error("failed to init curl multi handle");
    }
  }

  ~CurlMulti() noexcept {
    if (handle_) {
      ::curl_multi_cleanup(handle_);
    }
  }

  /// Return true on handle initialized
  [[nodiscard]] explicit operator bool() const noexcept {
    return handle_ != nullptr;
  }

  /// Return native handle
  [[nodiscard]] operator CURLM*() const noexcept {
    return handle_;
  }
};

} // namespace walng
