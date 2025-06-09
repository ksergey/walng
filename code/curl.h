// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <chrono>
#include <concepts>
#include <expected>
#include <optional>
#include <span>
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

template <typename T>
T getInfo(Easy const& handle, CURLINFO info) noexcept {
  T result;
  ::curl_easy_getinfo(handle, info, &result);
  return result;
}

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
  curl::Easy handle_;
  std::string body_;

public:
  Response(Response const&) = delete;
  Response& operator=(Response const&) = delete;
  Response(Response&&) = default;
  Response& operator=(Response&&) = default;
  Response() = default;

  Response(curl::Easy handle, std::string body) noexcept : handle_(std::move(handle)), body_(std::move(body)) {}

  [[nodiscard]] std::string const& body() const noexcept {
    return body_;
  }

  [[nodiscard]] std::chrono::microseconds totalTime() const noexcept {
    return std::chrono::microseconds(curl::getInfo<curl_off_t>(handle_, CURLINFO_TOTAL_TIME_T));
  }

  [[nodiscard]] std::string effectiveURL() const noexcept {
    return std::string(curl::getInfo<char*>(handle_, CURLINFO_EFFECTIVE_URL));
  }
};

class Request {
private:
  curl::Easy handle_;
  std::string body_;

public:
  Request(Request const&) = delete;
  Request& operator=(Request const&) = delete;

  Request(Request&& other) noexcept : handle_(std::move(other.handle_)), body_(std::move(other.body_)) {
    ::curl_easy_setopt(handle_, CURLOPT_WRITEDATA, this);
  }

  Request& operator=(Request&& other) noexcept {
    if (this != &other) {
      this->~Request();
      new (this) Request(std::move(other));
    }
    return *this;
  }

  Request(std::string const& url, std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    ::curl_easy_setopt(handle_, CURLOPT_URL, url.c_str());
    ::curl_easy_setopt(handle_, CURLOPT_TIMEOUT, static_cast<long>(timeout.count()));
    ::curl_easy_setopt(handle_, CURLOPT_WRITEFUNCTION, writeFn);
    ::curl_easy_setopt(handle_, CURLOPT_WRITEDATA, this);
    ::curl_easy_setopt(handle_, CURLOPT_FOLLOWLOCATION, 1L);
  }

private:
  void writeChunk(std::span<char const> chunk) {
    body_.append(chunk.data(), chunk.size());
  }

  static std::size_t writeFn(char const* data, std::size_t size, std::size_t nmemb, void* userdata) {
    auto const self = static_cast<Request*>(userdata);
    auto const chunk = std::span<char const>(data, size * nmemb);
    self->writeChunk(chunk);
    return chunk.size();
  }

  friend class Executor;
};

template <typename T>
concept ResponseHandler = requires(T handler, Response const& response) {
  { handler(response) };
};

class Executor {
private:
  curl::Multi handle_;

public:
  Executor(Executor const&) = delete;
  Executor& operator=(Executor const&) = delete;
  Executor() = default;

  static Result<Response> perform(Request&& request) noexcept {
    auto const rc = ::curl_easy_perform(request.handle_);
    if (rc == CURLE_OK) {
      return Response(std::move(request.handle_), std::move(request.body_));
    } else {
      return std::unexpected(curl::makeEasyErrorCode(rc));
    }
  }
};

} // namespace walng
