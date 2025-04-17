// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#pragma once

#include <filesystem>
#include <string>

#include <inja/inja.hpp>

namespace walng {

struct Theme {
  /// Load theme from YAML file, throws on error
  /// \see https://github.com/tinted-theming/schemes
  [[nodiscard]] static inja::json loadFromYAMLFile(std::filesystem::path const& path);
};

} // namespace walng
