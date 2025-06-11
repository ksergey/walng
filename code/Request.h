// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <string>

#include "Response.h"
#include "curl.h"

namespace walng {

enum RequestError { InternalError };

namespace detail {

struct RequestErrorCategory final : std::error_category {
  char const* name() const noexcept override {
    return "request-error";
  }
  std::string message(int ec) const override {
    switch (static_cast<RequestError>(ec)) {
    case RequestError::InternalError:
      return "internal error";
    default:
      break;
    }
    return "unknown error";
  }
};

inline RequestErrorCategory const& getRequestErrorCategory() noexcept {
  static RequestErrorCategory category;
  return category;
}

inline std::error_code makeRequestErrorCode(RequestError ec) {
  return std::error_code(static_cast<int>(ec), getRequestErrorCategory());
}

} // namespace detail

class Request {
private:
  curl::EasyHandle handle_;
  std::string body_;

public:
  Request(Request const&) = delete;
  Request& operator=(Request const&) = delete;

  Request() = default;

  Request(Request&& other) noexcept : handle_(std::move(other.handle_)), body_(std::move(other.body_)) {
    if (handle_) {
      handle_.option(CURLOPT_WRITEDATA, this);
    }
  }

  Request& operator=(Request&& other) noexcept {
    if (this != &other) {
      this->~Request();
      new (this) Request(std::move(other));
    }
    return *this;
  }

  std::expected<void, std::system_error> prepare(
      std::string const& url, std::chrono::milliseconds timeout = std::chrono::seconds(5)) noexcept {
    if (!handle_) {
      auto result = curl::EasyHandle::create();
      if (!result) {
        return std::unexpected(std::system_error(result.error().code(), "failed to init CURL easy handle"));
      }
      handle_ = std::move(*result);
    } else {
      handle_.reset();
    }

    handle_.option(CURLOPT_WRITEDATA, this);
    handle_.option(CURLOPT_WRITEFUNCTION, writeFn);
    handle_.option(CURLOPT_FOLLOWLOCATION, 1L);

    if (auto const rc = handle_.option(CURLOPT_URL, url); !rc) {
      return std::unexpected(std::system_error(rc.error().code(), "failed to set URL"));
    }
    if (auto const rc = handle_.option(CURLOPT_TIMEOUT_MS, static_cast<long>(timeout.count())); !rc) {
      return std::unexpected(std::system_error(rc.error().code(), "failed to set timeout"));
    }

    body_.clear();

    return {};
  }

  std::expected<Response, std::system_error> perform() noexcept {
    return handle_.perform()
        .transform([&] {
          return Response(std::move(handle_), std::move(body_));
        })
        .transform_error([&](std::system_error const& e) {
          return std::system_error(e.code(), "failed to execute request");
        });
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
