// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "colors.h"

namespace walng {

static_assert(parseColorFromHexStr("#3233ae") == Color{0x3233ae});
static_assert(parseColorFromHexStr("#xzxa2w") == std::nullopt);

} // namespace walng
