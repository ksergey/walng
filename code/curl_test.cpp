// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <print>
#include <string>

#include "NetThread.h"

namespace {

struct Handler final : walng::ResponseHandler {
  std::string content;
  bool done = false;

  void processWriteChunk(std::span<char const> chunk) override {
    content.append(chunk.data(), chunk.size());
  }

  void processComplete(int code, std::string_view text) override {
    std::print(stdout, "code: {}, text: \"{}\"\n", code, text);
    std::print(stdout, "{}\n", content);
    done = true;
  }
} handler;

} // namespace

void curl_test() {
  walng::NetThread netThread;

  netThread.enqueue({"wttr.in/Moscow"}, {}, &handler);

  while (true) {
    netThread.complete();

    if (handler.done) {
      break;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }
}
