// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <chrono>
#include <string>

#include "curl.h"

namespace walng {

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

} // namespace walng
