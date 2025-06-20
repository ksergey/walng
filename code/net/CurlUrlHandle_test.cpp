// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <doctest/doctest.h>

#include "CurlUrlHandle.h"

namespace walng::net {

TEST_CASE("CurlUrlHandle: move-semantic") {
  CurlUrlHandle handle;
  CHECK(!handle);

  auto result = CurlUrlHandle::create();
  CHECK(result);
  CHECK(*result);

  handle = std::move(*result);
  CHECK(handle);
  CHECK(!result.value());
}

TEST_CASE("CurlUrlHandle: ") {
  auto result = CurlUrlHandle::create().and_then([](auto&& result) -> std::expected<CurlUrlHandle, std::error_code> {
    if (auto rc = result.setPart(CURLUPART_URL, "https://github.com/ksergey/walng/README.md"); !rc) {
      return std::unexpected(rc.error());
    }
    return result;
  });

  CHECK(result);
  CurlUrlHandle handle = std::move(*result);

  if (auto rc = handle.part(CURLUPART_URL); rc) {
    CHECK(rc.value() == "https://github.com/ksergey/walng/README.md");
  } else {
    CHECK(false);
  }

  if (auto rc = handle.part(CURLUPART_PATH); rc) {
    CHECK(rc.value() == "/ksergey/walng/README.md");
  } else {
    CHECK(false);
  }

  if (auto rc = handle.setPart(CURLUPART_PATH, "/xyz"); !rc) {
    CHECK(false);
  }

  if (auto rc = handle.part(CURLUPART_PATH); rc) {
    CHECK(rc.value() == "/xyz");
  } else {
    CHECK(false);
  }

  if (auto rc = handle.part(CURLUPART_URL); rc) {
    CHECK(rc.value() == "https://github.com/xyz");
  } else {
    CHECK(false);
  }
}

} // namespace walng::net
