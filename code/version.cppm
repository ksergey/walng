// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

module;

#include <string_view>

export module walng.version;

namespace walng {

#if defined(WALNG_VERSION)
    export constexpr std::string_view version = WALNG_VERSION;
#else
    export constexpr std::string_view version = "N/A";
#endif

} // namespace
