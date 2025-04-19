// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <filesystem>

#include <inja/inja.hpp>

namespace walng {

/// Load base16 or base24 theme from yaml file
[[nodiscard]] inja::json loadBaseXXThemeFromYAMLFile(std::filesystem::path const& path);

} // namespace walng
