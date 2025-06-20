// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <system_error>

#include <curl/curl.h>

namespace walng::net {

enum class CurlInitError { EasyInit, UrlInit };

struct CurlInitErrorCategory final : std::error_category {
  char const* name() const noexcept override {
    return "curl-init-error";
  }
  std::string message(int ec) const override {
    switch (static_cast<CurlInitError>(ec)) {
    case CurlInitError::EasyInit:
      return "failed to init easy handle";
    case CurlInitError::UrlInit:
      return "failed to init url handle";
    default:
      return "(unrecognized error)";
    }
  }
};

CurlInitErrorCategory const theCurlInitErrorCategory{};

inline std::error_code make_error_code(CurlInitError e) noexcept {
  return {static_cast<int>(e), theCurlInitErrorCategory};
}

struct CurlEasyErrorCategory final : std::error_category {
  char const* name() const noexcept override {
    return "curl-easy-error";
  }
  std::string message(int ec) const override {
    return ::curl_easy_strerror(static_cast<::CURLcode>(ec));
  }
};

CurlEasyErrorCategory const theCurlEasyErrorCategory{};

inline std::error_code make_error_code(::CURLcode ec) {
  return std::error_code(static_cast<int>(ec), theCurlEasyErrorCategory);
}

struct CurlUrlErrorCategory final : std::error_category {
  char const* name() const noexcept override {
    return "curl-url-error";
  }
  std::string message(int ec) const override {
    return ::curl_url_strerror(static_cast<::CURLUcode>(ec));
  }
};

CurlUrlErrorCategory const theCurlUrlErrorCategory{};

inline std::error_code make_error_code(::CURLUcode ec) {
  return std::error_code(static_cast<int>(ec), theCurlUrlErrorCategory);
}

} // namespace walng::net
