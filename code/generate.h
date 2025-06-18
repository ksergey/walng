// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <filesystem>

namespace walng {

/// Generate colorschemes according to config and theme
void generate(std::filesystem::path const& configPath, std::filesystem::path const& themePath);

} // namespace walng
