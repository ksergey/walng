// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <atomic>
#include <flat_set>
#include <thread>

#include "curl.h"

namespace walng {

class FetchWorker {
private:
  std::atomic_bool running_ = true;
  curl::MultiHandle multiHandle_;
  std::thread thread_;

  std::flat_set<CURL*> pendingEnqueueTransfers_;
  std::flat_set<CURL*> activeTransfers_;

public:
  FetchWorker(FetchWorker const&) = delete;
  FetchWorker& operator=(FetchWorker const&) = delete;

  FetchWorker();
  ~FetchWorker() noexcept;

private:
  void finishTransfer(CURL* easyHandle, CURLcode code) noexcept;
  void abortAllTransfers() noexcept;

  void processPending() noexcept;
  void processTransfer() noexcept;
  void processCompleted() noexcept;
  void waitTransferEvents() noexcept;

  void loop() noexcept;
};

} // namespace walng
