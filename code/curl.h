// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <utility>

#include <curl/curl.h>

namespace walng::curl {

/// Wrapper for curl multi handle
class MultiHandle {
private:
  CURLM* handle_ = nullptr;

public:
  MultiHandle(MultiHandle const&) = delete;
  MultiHandle& operator=(MultiHandle const&) = delete;

  MultiHandle(MultiHandle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  MultiHandle& operator=(MultiHandle&& other) noexcept {
    if (this != &other) {
      this->~MultiHandle();
      new (this) MultiHandle(std::move(other));
    }
    return *this;
  }

  MultiHandle() {
    handle_ = ::curl_multi_init();
    if (handle_ == nullptr) {
      throw std::runtime_error("failed to init curl multi handle");
    }
  }

  ~MultiHandle() noexcept {
    if (handle_) {
      ::curl_multi_cleanup(handle_);
    }
  }

  /// Return true on handle initialized
  [[nodiscard]] explicit operator bool() const noexcept {
    return handle_ != nullptr;
  }

  /// Return native handle
  [[nodiscard]] operator CURLM*() const noexcept {
    return handle_;
  }
};

} // namespace walng::curl
