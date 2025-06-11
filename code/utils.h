// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <filesystem>
#include <stdexcept>

namespace walng {

/// Get path to $HOME directory
/// @throw std::runtime_error on $HOME not set
std::filesystem::path const& homePath();

/// Get path to \c walng config directory
/// @throw std::runtime_error on $HOME not set
///
/// $XDG_CONFIG_HOME/walng
/// $HOME/.config/walng
std::filesystem::path const& configPathBase();

/// Get path to \c walng cache directory
/// @throw std::runtime_error on $HOME not set
///
/// $XDG_CACHE_HOME/walng
/// $HOME/.cache/walng
std::filesystem::path const& cacheBasePath();

/// Expand @c ~ to $HOME
void expandTilda(std::filesystem::path& path);

/// Make temporary filename
std::filesystem::path makeTemp();

} // namespace walng
