// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <string>

#include "Response.h"
#include "curl.h"

namespace walng {

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
      handle_.setOption(CURLOPT_HTTPGET, 1L);
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
      return curl::detail::makeUnexpectedEasyError(rc);
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
