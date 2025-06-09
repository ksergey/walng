// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <expected>
#include <system_error>
#include <utility>

#include <curl/curl.h>

namespace walng {

template <typename T = void, typename E = std::error_code>
using Result = std::expected<T, E>;

namespace curl {

struct EasyErrorCategory final : std::error_category {
  char const* name() const noexcept override {
    return "curl-easy";
  }
  std::string message(int ec) const override {
    return ::curl_easy_strerror(static_cast<::CURLcode>(ec));
  }
};

inline std::error_code makeEasyErrorCode(CURLcode rc) noexcept {
  static EasyErrorCategory errorCategory;
  return std::error_code(static_cast<int>(rc), errorCategory);
}

struct MultiErrorCategory final : std::error_category {
  char const* name() const noexcept override {
    return "multi-easy";
  }
  std::string message(int ec) const override {
    return ::curl_multi_strerror(static_cast<::CURLMcode>(ec));
  }
};

inline std::error_code makeMultiErrorCode(CURLMcode rc) noexcept {
  static MultiErrorCategory errorCategory;
  return std::error_code(static_cast<int>(rc), errorCategory);
}

class Easy {
private:
  CURL* handle_ = nullptr;

public:
  Easy(Easy const&) = delete;
  Easy& operator=(Easy const&) = delete;

  Easy() {
    handle_ = ::curl_easy_init();
    if (!handle_) [[unlikely]] {
      throw std::system_error(ENOMEM, std::generic_category(), "failed to init easy handle");
    }
  }

  ~Easy() noexcept {
    if (handle_) {
      ::curl_easy_cleanup(handle_);
    }
  }

  Easy(Easy&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  Easy& operator=(Easy&& other) noexcept {
    if (this != &other) {
      this->~Easy();
      new (this) Easy(std::move(other));
    }
    return *this;
  }

  [[nodiscard]] operator CURL*() const noexcept {
    return handle_;
  }
};

class Multi {
private:
  CURLM* handle_ = nullptr;

public:
  Multi(Multi const&) = delete;
  Multi& operator=(Multi const&) = delete;

  Multi() {
    handle_ = ::curl_multi_init();
    if (!handle_) [[unlikely]] {
      throw std::system_error(ENOMEM, std::generic_category(), "failed to init multi handle");
    }
  }

  Multi(Multi&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  Multi& operator=(Multi&& other) noexcept {
    if (this != &other) {
      this->~Multi();
      new (this) Multi(std::move(other));
    }
    return *this;
  }

  ~Multi() noexcept {
    if (handle_) {
      ::curl_multi_cleanup(handle_);
    }
  }

  [[nodiscard]] operator CURLM*() const noexcept {
    return handle_;
  }
};

} // namespace curl

class Response {
private:
public:
  Response(Response const&) = delete;
  Response& operator=(Response const&) = delete;
  Response() = default;
};

class Request {
private:
public:
  Request(Request const&) = delete;
  Request& operator=(Request const&) = delete;
  Request() = default;
};

class Executor {
private:
  curl::Multi handle_;

public:
  Executor(Executor const&) = delete;
  Executor& operator=(Executor const&) = delete;
  Executor() = default;
};

} // namespace walng
