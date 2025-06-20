// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <system_error>

#include <curl/curl.h>

namespace walng::net {

enum class CurlError { EasyInit, UrlInit, EasySetOpt, EasyGetInfo, UrlGet, UrlSet };

struct CurlErrorCategory final : std::error_category {
  char const* name() const noexcept override {
    return "curl-error";
  }
  std::string message(int ec) const override {
    switch (static_cast<CurlError>(ec)) {
    case CurlError::EasyInit:
      return "failed to init easy handle";
    case CurlError::UrlInit:
      return "failed to init url handle";
    case CurlError::EasySetOpt:
      return "failed to set easy opt";
    case CurlError::EasyGetInfo:
      return "failed to get easy info";
    case CurlError::UrlGet:
      return "failed to get url part";
    case CurlError::UrlSet:
      return "failed to set url part";
    default:
      return "(unrecognized error)";
    }
  }
};

CurlErrorCategory const theCurlErrorCategory{};

inline std::error_code make_error_code(CurlError e) noexcept {
  return {static_cast<int>(e), theCurlErrorCategory};
}

} // namespace walng::net
