// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <atomic>
#include <print>
#include <string>

#include "NetThread.h"

namespace {

class Handler final : public walng::ResponseHandler {
private:
  std::string content_;
  std::atomic<bool> done_ = false;

public:
  void processWriteChunk(std::span<char const> chunk) override {
    content_.append(chunk.data(), chunk.size());
  }

  void processComplete(int code, std::string_view text) override {
    std::print(stdout, "code: {}, text: \"{}\"\n", code, text);
    std::print(stdout, "{}\n", content_);
    done_ = true;
  }

  bool done() const noexcept {
    return done_.load(std::memory_order_relaxed);
  }
};

} // namespace

void curl_test() {
  walng::NetThread netThread;

  Handler moscow;
  netThread.enqueue({"wttr.in/Moscow"}, {}, &moscow);

  Handler brest;
  netThread.enqueue({"wttr.in/Brest"}, {}, &brest);

  while (true) {
    netThread.complete();

    if (moscow.done() && brest.done()) {
      break;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }
}
