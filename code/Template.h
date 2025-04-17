// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <filesystem>

#include <inja/inja.hpp>

namespace walng {

struct Template {
  static void render(inja::json const& theme, std::filesystem::path const& templatePath);
};

} // namespace walng
