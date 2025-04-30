// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string_view>

namespace walng {

#if defined(WALNG_VERSION)
    constexpr std::string_view version = WALNG_VERSION;
#else
    constexpr std::string_view version = "N/A";
#endif

} // namespace
