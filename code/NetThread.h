// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <curl/curl.h>

namespace walng {
namespace detail {

/// Wrapper for curl easy handle
class CurlEasy {
private:
  CURL* handle_ = nullptr;

public:
  CurlEasy(CurlEasy const&) = delete;
  CurlEasy& operator=(CurlEasy const&) = delete;

  CurlEasy(CurlEasy&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  CurlEasy& operator=(CurlEasy&& other) noexcept {
    if (this != &other) {
      this->~CurlEasy();
      new (this) CurlEasy(std::move(other));
    }
    return *this;
  }

  CurlEasy() {
    handle_ = ::curl_easy_init();
    if (handle_ == nullptr) {
      throw std::runtime_error("failed to init curl multi handle");
    }
  }

  ~CurlEasy() noexcept {
    if (handle_) {
      ::curl_easy_cleanup(handle_);
    }
  }

  /// Return true on handle initialized
  [[nodiscard]] explicit operator bool() const noexcept {
    return handle_ != nullptr;
  }

  /// Return native handle
  [[nodiscard]] operator CURL*() const noexcept {
    return handle_;
  }
};

/// Wrapper for curl multi handle
class CurlMulti {
private:
  CURLM* handle_ = nullptr;

public:
  CurlMulti(CurlMulti const&) = delete;
  CurlMulti& operator=(CurlMulti const&) = delete;

  CurlMulti(CurlMulti&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  CurlMulti& operator=(CurlMulti&& other) noexcept {
    if (this != &other) {
      this->~CurlMulti();
      new (this) CurlMulti(std::move(other));
    }
    return *this;
  }

  CurlMulti() {
    handle_ = ::curl_multi_init();
    if (handle_ == nullptr) {
      throw std::runtime_error("failed to init curl multi handle");
    }
  }

  ~CurlMulti() noexcept {
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

} // namespace detail

class NetThread;

struct Request {
  std::string url;
  std::string userAgent = "NetThread/curl";
};

struct CancellationToken {
  std::size_t id;
};

/// Response handler which executed inside NetThread background thread
struct ResponseHandler {
  virtual ~ResponseHandler() = default;

  virtual void processWriteChunk(std::span<char const> chunk) = 0;
  virtual void processComplete(int code, std::string_view text) = 0;
};

class NetThread {
private:
  class RequestState;
  using RequestStatePtr = std::unique_ptr<RequestState>;

  std::thread thread_;
  std::atomic<bool> threadRunningFlag_ = true;

  detail::CurlMulti multiHandle_;

  std::mutex pendingMutex_;
  std::vector<RequestStatePtr> pendingEnqueue_;
  std::vector<CancellationToken> pendingCancel_;

  std::mutex completedMutex_;
  std::vector<RequestStatePtr> completed_;

public:
  NetThread(NetThread const&) = delete;
  NetThread& operator=(NetThread const&) = delete;

  NetThread();
  ~NetThread();

  /// Enqueue request for execution
  void enqueue(Request const& request, CancellationToken cancellationToken, ResponseHandler* responseHandler);

  /// Cancel request
  void cancel(CancellationToken cancellationToken);

  /// Invoke complete callback
  // TODO: real need?
  void complete();

private:
  void worker();

  void worker_processPending();
  void worker_processCompleted();
  void worker_enqueue(RequestStatePtr state);
  void worker_complete(RequestStatePtr state, int code);
};

} // namespace walng
