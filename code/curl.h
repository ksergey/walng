// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <expected>
#include <span>
#include <system_error>
#include <utility>

#include <curl/curl.h>

namespace walng::curl {

enum class InternalError { FailedEasyHandle, FailedUrlHandle };

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

inline std::error_code makeEasyErrorCode(::CURLcode ec) {
  return std::error_code(static_cast<int>(ec), getEasyErrorCategory());
}

struct UrlErrorCategory final : std::error_category {
  char const* name() const noexcept override {
    return "curl-url";
  }
  std::string message(int ec) const override {
    return ::curl_url_strerror(static_cast<::CURLUcode>(ec));
  }
};

inline UrlErrorCategory const& getUrlErrorCategory() noexcept {
  static UrlErrorCategory category;
  return category;
}

inline std::error_code makeUrlErrorCode(::CURLUcode ec) {
  return std::error_code(static_cast<int>(ec), getUrlErrorCategory());
}

struct InternalErrorCategory final : std::error_category {
  char const* name() const noexcept override {
    return "internal-error";
  }
  std::string message(int ec) const override {
    switch (static_cast<InternalError>(ec)) {
    case InternalError::FailedEasyHandle:
      return "failed to init easy handle";
    case InternalError::FailedUrlHandle:
      return "failed to init url handle";
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

inline std::error_code makeInternalErrorCode(InternalError ec) {
  return std::error_code(static_cast<int>(ec), getInternalErrorCategory());
}

} // namespace detail

class EasyHandle {
private:
  CURL* handle_ = nullptr;

public:
  EasyHandle(EasyHandle const&) = delete;
  EasyHandle& operator=(EasyHandle const&) = delete;

  EasyHandle() = default;

  ~EasyHandle() noexcept {
    if (handle_) {
      ::curl_easy_cleanup(handle_);
    }
  }

  EasyHandle(EasyHandle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  EasyHandle& operator=(EasyHandle&& other) noexcept {
    if (this != &other) {
      this->~EasyHandle();
      new (this) EasyHandle(std::move(other));
    }
    return *this;
  }

  explicit EasyHandle(CURL* handle) noexcept : handle_(handle) {}

  static std::expected<EasyHandle, std::system_error> create() noexcept {
    auto handle = ::curl_easy_init();
    if (!handle) [[unlikely]] {
      return std::unexpected(detail::makeInternalErrorCode(InternalError::FailedEasyHandle));
    }
    return EasyHandle(handle);
  }

  [[nodiscard]] operator bool() const noexcept {
    return handle_ != nullptr;
  }

  void reset() noexcept {
    ::curl_easy_reset(handle_);
  }

  template <typename T>
  std::expected<void, std::system_error> option(CURLoption opt, T&& value) noexcept {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
      return this->optionImpl(opt, value.c_str());
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>) {
      return this->optionImpl(opt, std::string(value).c_str());
    } else {
      return this->optionImpl(opt, std::forward<T>(value));
    }
  }

  template <typename T>
  std::expected<T, std::system_error> info(CURLINFO info) const noexcept {
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

  std::expected<void, std::system_error> perform() noexcept {
    if (auto const rc = ::curl_easy_perform(handle_); rc != CURLE_OK) [[unlikely]] {
      return std::unexpected(detail::makeEasyErrorCode(rc));
    }
    return {};
  }

private:
  template <typename T>
  inline std::expected<void, std::system_error> optionImpl(CURLoption opt, T&& value) noexcept {
    if (auto const rc = ::curl_easy_setopt(handle_, opt, std::forward<T>(value)); rc != CURLE_OK) [[unlikely]] {
      return std::unexpected(detail::makeEasyErrorCode(rc));
    }
    return {};
  }

  template <typename T>
  inline std::expected<T, std::system_error> infoImpl(CURLINFO info) const noexcept {
    T result;
    if (auto const rc = ::curl_easy_getinfo(handle_, info, &result); rc != CURLE_OK) [[unlikely]] {
      return std::unexpected(detail::makeEasyErrorCode(rc));
    }
    return result;
  }
};

class UrlPart {
private:
  char* value_ = nullptr;

public:
  UrlPart(UrlPart const&) = delete;
  UrlPart& operator=(UrlPart const&) = delete;
  UrlPart() = default;

  UrlPart(UrlPart&& other) noexcept : value_(std::exchange(other.value_, nullptr)) {}

  UrlPart& operator=(UrlPart&& other) noexcept {
    if (this != &other) {
      value_ = std::exchange(other.value_, nullptr);
    }
    return *this;
  }

  explicit UrlPart(char* value) noexcept : value_(value) {}

  ~UrlPart() {
    if (value_) {
      ::curl_free(value_);
    }
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return value_ != nullptr;
  }

  [[nodiscard]] char const* c_str() const noexcept {
    return value_ ? value_ : "";
  }

  [[nodiscard]] operator std::string_view() const noexcept {
    return c_str();
  }
};

class UrlHandle {
private:
  CURLU* handle_ = nullptr;

public:
  UrlHandle(UrlHandle const&) = delete;
  UrlHandle& operator=(UrlHandle const&) = delete;

  UrlHandle() = default;

  ~UrlHandle() noexcept {
    if (handle_) {
      ::curl_url_cleanup(handle_);
    }
  }

  UrlHandle(UrlHandle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  UrlHandle& operator=(UrlHandle&& other) noexcept {
    if (this != &other) {
      this->~UrlHandle();
      new (this) UrlHandle(std::move(other));
    }
    return *this;
  }

  explicit UrlHandle(CURLU* handle) noexcept : handle_(handle) {}

  static std::expected<UrlHandle, std::system_error> create() noexcept {
    auto handle = ::curl_url();
    if (!handle) [[unlikely]] {
      return std::unexpected(detail::makeInternalErrorCode(InternalError::FailedUrlHandle));
    }
    return UrlHandle(handle);
  }

  [[nodiscard]] operator bool() const noexcept {
    return handle_ != nullptr;
  }

  std::expected<UrlPart, std::system_error> getPart(CURLUPart part) noexcept {
    return getPartImpl(part);
  }

  template <typename T>
  std::expected<void, std::system_error> setPart(CURLUPart part, T&& value) noexcept {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
      return setPartImpl(part, value.c_str());
    } else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>) {
      return setPartImpl(part, std::string(value).c_str());
    } else {
      return setPartImpl(part, value);
    }
  }

private:
  inline std::expected<UrlPart, std::system_error> getPartImpl(CURLUPart part) const noexcept {
    char* value = nullptr;
    if (auto const rc = ::curl_url_get(handle_, part, &value, 0); rc != CURLUE_OK) [[unlikely]] {
      return std::unexpected(detail::makeUrlErrorCode(rc));
    }
    return UrlPart(value);
  }

  inline std::expected<void, std::system_error> setPartImpl(CURLUPart part, char const* value) noexcept {
    if (auto const rc = ::curl_url_set(handle_, part, value, 0); rc != CURLUE_OK) [[unlikely]] {
      return std::unexpected(detail::makeUrlErrorCode(rc));
    }
    return {};
  }
};

} // namespace walng::curl
