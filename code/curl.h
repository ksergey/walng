// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <expected>
#include <span>
#include <system_error>
#include <utility>

#include <curl/curl.h>

namespace walng::curl {

enum class InternalError { FailedEasyHandle };

namespace detail {

struct EasyErrorCategory final : std::error_category {
  char const* name() const noexcept override {
    return "curl-easy";
  }
  std::string message(int ec) const override {
    return ::curl_easy_strerror(static_cast<::CURLcode>(ec));
  }
};

inline EasyErrorCategory const& getEasyErrorCategory() noexcept {
  static EasyErrorCategory category;
  return category;
}

inline decltype(auto) makeUnexpectedEasyError(CURLcode ec) noexcept {
  return std::unexpected(std::error_code(static_cast<int>(ec), getEasyErrorCategory()));
}

struct InternalErrorCategory final : std::error_category {
  char const* name() const noexcept override {
    return "internal-error";
  }
  std::string message(int ec) const override {
    switch (static_cast<InternalError>(ec)) {
    case InternalError::FailedEasyHandle:
      return "failed to init easy handle";
    default:
      break;
    }
    return "";
  }
};

inline InternalErrorCategory const& getInternalErrorCategory() noexcept {
  static InternalErrorCategory category;
  return category;
}

inline decltype(auto) makeUnexpectedInternalError(InternalError ec) {
  return std::unexpected(std::error_code(static_cast<int>(ec), getInternalErrorCategory()));
}

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
    if constexpr (std::is_same_v<T, std::string const&>) {
      return setOption(option, parameter.c_str());
    } else if constexpr (std::is_same_v<T, std::string_view>) {
      return setOption(option, std::string(parameter).c_str());
    } else {
      if (auto const rc = ::curl_easy_setopt(handle_, option, parameter); rc != CURLE_OK) {
        return detail::makeUnexpectedEasyError(rc);
      }
    }
    return {};
  }

  template <typename T>
  std::expected<T, std::error_code> getInfo(CURLINFO info) const noexcept {
    if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
      return getInfo<char const*>(info);
    } else {
      T result;
      if (auto const rc = ::curl_easy_getinfo(handle_, info, &result); rc != CURLE_OK) {
        return detail::makeUnexpectedEasyError(rc);
      }
      return result;
    }
  }

  static std::expected<Easy, std::error_code> create() noexcept {
    auto handle = ::curl_easy_init();
    if (!handle) [[unlikely]] {
      return detail::makeUnexpectedInternalError(InternalError::FailedEasyHandle);
    }
    return Easy(handle);
  }
};

} // namespace walng::curl
