// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <doctest/doctest.h>

#include "CurlEasyHandle.h"

namespace walng::net {

TEST_CASE("CurlEasyHandle: move-semantic") {
  CurlEasyHandle handle;
  CHECK(!handle);
  CHECK(!handle.setOption(CURLOPT_FOLLOWLOCATION, 1U));

  auto result = CurlEasyHandle::create();
  CHECK(result);
  CHECK(*result);

  handle = std::move(*result);
  CHECK(handle);
  CHECK(!result.value());

  CHECK(handle.setOption(CURLOPT_FOLLOWLOCATION, 1U));
}

} // namespace walng::net
