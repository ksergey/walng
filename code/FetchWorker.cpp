// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "FetchWorker.h"

#include <print>

namespace walng {

FetchWorker::FetchWorker() {
  thread_ = std::thread([this] {
    loop();
  });
}

FetchWorker::~FetchWorker() noexcept {
  running_.store(false, std::memory_order_relaxed);

  ::curl_multi_wakeup(multiHandle_);

  if (thread_.joinable()) {
    thread_.join();
  }
}

void FetchWorker::finishTransfer(CURL* easyHandle, CURLcode code) noexcept {
  activeTransfers_.erase(easyHandle);
  ::curl_multi_remove_handle(multiHandle_, easyHandle);
  ::curl_easy_cleanup(easyHandle);
}

void FetchWorker::abortAllTransfers() noexcept {
  for (auto const easyHandle : activeTransfers_) {
    ::curl_multi_remove_handle(multiHandle_, easyHandle);
    ::curl_easy_cleanup(easyHandle);
  }
  activeTransfers_.clear();
}

void FetchWorker::processPending() noexcept {
  // FIXME
}

void FetchWorker::processTransfer() noexcept {
  int active = 0;
  auto const rc = ::curl_multi_perform(multiHandle_, &active);
  if (rc != CURLM_OK) [[unlikely]] {
    abortAllTransfers();
  }
}

void FetchWorker::processCompleted() noexcept {
  int pending = 0;

  while (true) {
    auto const msg = ::curl_multi_info_read(multiHandle_, &pending);
    if (!msg) {
      break;
    }
    if (msg->msg != CURLMSG_DONE) [[unlikely]] {
      continue;
    }
    finishTransfer(msg->easy_handle, msg->data.result);
  }
}

void FetchWorker::waitTransferEvents() noexcept {
  constexpr int kWaitTimeoutMs = 10000;

  int numfds = 0;
  auto const rc = ::curl_multi_poll(multiHandle_, nullptr, 0, kWaitTimeoutMs, &numfds);
  if (rc != CURLM_OK) [[unlikely]] {
    std::print(stderr, "curl_multi_poll failed ({})\n", curl_multi_strerror(rc));
  }
}

void FetchWorker::loop() noexcept {
  while (running_.load(std::memory_order_relaxed)) {
    processPending();
    processTransfer();
    processCompleted();
    waitTransferEvents();
  }
  std::print("bye\n");
}

} // namespace walng
