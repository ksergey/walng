// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include "curl.h"

namespace walng {

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

  CurlMulti multiHandle_;

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
