// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include <doctest/doctest.h>

#include "error.h"

namespace walng::net {

TEST_CASE("net-error: CurlInit") {
  CHECK(make_error_code(CurlInitError::UrlInit).message() == "failed to init url handle");
}

} // namespace walng::net
