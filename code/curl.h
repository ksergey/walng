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

enum class InternalError { FailedEasyHandle, FailedMultiHandle };

struct InternalErrorCategory final : std::error_category {
  char const* name() const noexcept override {
    return "internal-error";
  }
  std::string message(int ec) const override {
    switch (static_cast<InternalError>(ec)) {
    case InternalError::FailedEasyHandle:
      return "failed to init easy handle";
    case InternalError::FailedMultiHandle:
      return "failed to init multi handle";
    default:
      break;
    }
    return "";
  }
};

inline std::error_code makeInternalErrorCode(InternalError rc) noexcept {
  static InternalErrorCategory errorCategory;
  return std::error_code(static_cast<int>(rc), errorCategory);
}

namespace detail {

template <typename T>
struct GetInfoImpl {
  std::expected<T, std::error_code> operator()(CURL* handle, CURLINFO info) const noexcept {
    T result;
    if (auto const rc = ::curl_easy_getinfo(handle, info, &result); rc != CURLE_OK) {
      return std::unexpected(makeEasyErrorCode(rc));
    }
    return result;
  }
};

template <>
struct GetInfoImpl<std::string> {
  std::expected<std::string, std::error_code> operator()(CURL* handle, CURLINFO info) const noexcept {
    return GetInfoImpl<char const*>{}(handle, info).transform([](char const* value) {
      return std::string(value);
    });
  }
};

template <>
struct GetInfoImpl<std::string_view> {
  std::expected<std::string_view, std::error_code> operator()(CURL* handle, CURLINFO info) const noexcept {
    return GetInfoImpl<char const*>{}(handle, info).transform([](char const* value) {
      return std::string_view(value);
    });
  }
};

} // namespace detail

class Easy {
private:
  CURL* handle_ = nullptr;

public:
  Easy(Easy const&) = delete;
  Easy& operator=(Easy const&) = delete;

  Easy() = default;

  explicit Easy(CURL* handle) noexcept : handle_(handle) {}

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

  void reset() noexcept {
    ::curl_easy_reset(handle_);
  }

  template <typename T>
  std::expected<void, std::error_code> setOption(CURLoption option, T parameter) noexcept {
    if (auto const rc = ::curl_easy_setopt(handle_, option, parameter); rc != CURLE_OK) {
      return std::unexpected(makeEasyErrorCode(rc));
    }
    return {};
  }

  std::expected<void, std::error_code> setOption(CURLoption option, std::string const& parameter) noexcept {
    return setOption(option, parameter.c_str());
  }

  std::expected<void, std::error_code> setOption(CURLoption option, std::string_view parameter) noexcept {
    return setOption(option, std::string(parameter).c_str());
  }

  template <typename T>
  std::expected<T, std::error_code> getInfo(CURLINFO info) const noexcept {
    return detail::GetInfoImpl<T>{}(handle_, info);
  }

  static std::expected<Easy, std::error_code> create() noexcept {
    auto handle = ::curl_easy_init();
    if (!handle) [[unlikely]] {
      return std::unexpected(makeInternalErrorCode(InternalError::FailedEasyHandle));
    }
    return Easy(handle);
  }
};

class Multi {
private:
  CURLM* handle_ = nullptr;

public:
  Multi(Multi const&) = delete;
  Multi& operator=(Multi const&) = delete;

  Multi() = default;

  explicit Multi(CURLM* handle) noexcept : handle_(handle) {}

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

  static std::expected<Multi, std::error_code> create() noexcept {
    auto handle = ::curl_multi_init();
    if (!handle) [[unlikely]] {
      return std::unexpected(makeInternalErrorCode(InternalError::FailedMultiHandle));
    }
    return Multi(handle);
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

  [[nodiscard]] std::chrono::microseconds totalTime() const {
    return handle_.getInfo<curl_off_t>(CURLINFO_TOTAL_TIME_T)
        .transform([](curl_off_t value) {
          return std::chrono::microseconds(value);
        })
        .value();
  }

  [[nodiscard]] std::string_view effectiveURL() const {
    return handle_.getInfo<std::string_view>(CURLINFO_EFFECTIVE_URL).value();
  }
};

class Request {
private:
  curl::Easy handle_;
  std::string body_;

private:
  explicit Request(curl::Easy handle) noexcept : handle_(std::move(handle)) {
    if (handle_) {
      handle_.setOption(CURLOPT_WRITEDATA, this);
      handle_.setOption(CURLOPT_WRITEFUNCTION, writeFn);
      handle_.setOption(CURLOPT_FOLLOWLOCATION, 1L);
    }
  }

public:
  Request(Request const&) = delete;
  Request& operator=(Request const&) = delete;

  Request() = default;

  Request(Request&& other) noexcept : handle_(std::move(other.handle_)), body_(std::move(other.body_)) {
    if (handle_) {
      handle_.setOption(CURLOPT_WRITEDATA, this);
    }
  }

  Request& operator=(Request&& other) noexcept {
    if (this != &other) {
      this->~Request();
      new (this) Request(std::move(other));
    }
    return *this;
  }

  static std::expected<Request, std::error_code> create() noexcept {
    return curl::Easy::create().transform([](curl::Easy handle) {
      return Request(std::move(handle));
    });
  }

  std::expected<void, std::error_code> setURL(std::string const& value) noexcept {
    return handle_.setOption(CURLOPT_URL, value);
  }

  std::expected<void, std::error_code> setTimeout(std::chrono::milliseconds value) noexcept {
    return handle_.setOption(CURLOPT_TIMEOUT_MS, static_cast<long>(value.count()));
  }

  std::expected<Response, std::error_code> perform() noexcept {
    if (auto const rc = ::curl_easy_perform(handle_); rc != CURLE_OK) {
      return std::unexpected(curl::makeEasyErrorCode(rc));
    }
    return Response{std::move(handle_), std::move(body_)};
  }

  void reset() noexcept {
    handle_.reset();
    body_.clear();
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
};

} // namespace walng
