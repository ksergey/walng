// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "NetThread.h"

#include <print>

namespace walng {

class NetThread::RequestState {
private:
  CurlEasy easyHandle_;
  CancellationToken cancellationToken_;
  ResponseHandler* responseHandler_ = nullptr;
  int completeCode_ = 0;

public:
  RequestState(RequestState const&) = delete;
  RequestState& operator=(RequestState const&) = delete;
  RequestState(RequestState&&) = delete;
  RequestState& operator=(RequestState&&) = delete;

  RequestState() {
    ::curl_easy_setopt(easyHandle_, CURLOPT_FOLLOWLOCATION, 1L);
    ::curl_easy_setopt(easyHandle_, CURLOPT_WRITEDATA, this);
    ::curl_easy_setopt(easyHandle_, CURLOPT_WRITEFUNCTION, &writeCallback);
  }

  [[nodiscard]] CurlEasy const& handle() const noexcept {
    return easyHandle_;
  }

  [[nodiscard]] CancellationToken const& cancellationToken() const noexcept {
    return cancellationToken_;
  }

  [[nodiscard]] ResponseHandler* responseHandler() const noexcept {
    return responseHandler_;
  }

  [[nodiscard]] int completeCode() const noexcept {
    return completeCode_;
  }

  void setCompleteCode(int value) noexcept {
    completeCode_ = value;
  }

  void prepare(Request const& request, CancellationToken cancellationToken, ResponseHandler* responseHandler) {
    ::curl_easy_setopt(easyHandle_, CURLOPT_URL, request.url.c_str());
    ::curl_easy_setopt(easyHandle_, CURLOPT_USERAGENT, request.userAgent.c_str());

    cancellationToken_ = cancellationToken;
    responseHandler_ = responseHandler;
  }

private:
  static std::size_t writeCallback(char const* data, std::size_t size, std::size_t nmemb, void* userdata) {
    auto const self = static_cast<RequestState*>(userdata);
    auto const chunk = std::span<char const>(data, size * nmemb);
    self->responseHandler()->processWriteChunk(chunk);
    return chunk.size();
  }
};

NetThread::NetThread() {
  pendingEnqueue_.reserve(16);
  pendingCancel_.reserve(16);
  completed_.reserve(16);

  thread_ = std::thread([this] {
    worker();
  });
}

NetThread::~NetThread() {
  threadRunningFlag_ = false;

  ::curl_multi_wakeup(multiHandle_);

  if (thread_.joinable()) {
    thread_.join();
  }
}

void NetThread::enqueue(Request const& request, CancellationToken cancellationToken, ResponseHandler* responseHandler) {
  auto requestState = std::make_unique<RequestState>();
  requestState->prepare(request, cancellationToken, responseHandler);

  auto lock = std::unique_lock(pendingMutex_);
  pendingEnqueue_.push_back(std::move(requestState));

  ::curl_multi_wakeup(multiHandle_);
}

void NetThread::cancel(CancellationToken cancellationToken) {
  auto lock = std::unique_lock(pendingMutex_);
  pendingCancel_.push_back(std::move(cancellationToken));

  ::curl_multi_wakeup(multiHandle_);
}

void NetThread::complete() {
  auto lock = std::unique_lock(completedMutex_);

  if (!completed_.empty()) {
    for (auto& requestState : completed_) {
      requestState->responseHandler()->processComplete(requestState->completeCode(), "");
    }
    completed_.clear();
  }
}

void NetThread::worker() {
  try {
    while (threadRunningFlag_.load(std::memory_order_relaxed)) {
      worker_processPending();

      int numfds = 0;
      if (auto const rc = ::curl_multi_poll(multiHandle_, nullptr, 0, 20000, &numfds); rc != CURLM_OK) [[unlikely]] {
        std::print(stderr, "NetThread: curl_multi_poll error ({})\n", ::curl_multi_strerror(rc));
      }

      int runningHandles = 0;
      if (auto const rc = ::curl_multi_perform(multiHandle_, &runningHandles); rc != CURLM_OK) [[unlikely]] {
        std::print(stderr, "NetThread: curl_multi_perform error ({})\n", ::curl_multi_strerror(rc));
      }

      worker_processCompleted();
    }
  } catch (std::exception const& e) {
    std::print(stderr, "NetThread worker thread error: {}\n", e.what());
  }
}

void NetThread::worker_processPending() {
  auto lock = std::unique_lock(pendingMutex_);

  if (!pendingEnqueue_.empty()) {
    for (auto& requestState : pendingEnqueue_) {
      worker_enqueue(std::move(requestState));
    }
    pendingEnqueue_.clear();
  }

  if (!pendingCancel_.empty()) {
    for ([[maybe_unused]] auto& cancellationToken : pendingCancel_) {
      // not implemeted
    }
    pendingCancel_.clear();
  }
}

void NetThread::worker_processCompleted() {
  int messagesInQueue;
  while (auto const msg = ::curl_multi_info_read(multiHandle_, &messagesInQueue)) {
    if (msg->msg != CURLMSG_DONE) {
      continue;
    }

    auto const code = msg->data.result;
    auto const easyHandle = msg->easy_handle;

    // take ownership
    auto state = [easyHandle] {
      RequestState* state = nullptr;
      ::curl_easy_getinfo(easyHandle, CURLINFO_PRIVATE, &state);
      return std::unique_ptr<RequestState>(state);
    }();

    worker_complete(std::move(state), code);
  }
}

void NetThread::worker_enqueue(RequestStatePtr state) {
  ::curl_easy_setopt(state->handle(), CURLOPT_PRIVATE, state.get());
  ::curl_multi_add_handle(multiHandle_, state->handle());

  // ownership transfered
  state.release();
}

void NetThread::worker_complete(RequestStatePtr state, int code) {
  ::curl_multi_remove_handle(multiHandle_, state->handle());
  ::curl_easy_setopt(state->handle(), CURLOPT_PRIVATE, nullptr);

  state->setCompleteCode(code);

  auto lock = std::unique_lock(completedMutex_);
  completed_.push_back(std::move(state));
}

} // namespace walng
