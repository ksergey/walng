// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <expected>
#include <filesystem>
#include <stdexcept>

namespace walng {

/// Get path to $HOME directory
std::expected<std::filesystem::path, std::error_code> getHomePath();

/// Get path to \c walng config directory
///
/// One of:
///  - $XDG_CONFIG_HOME/walng
///  - $HOME/.config/walng
std::expected<std::filesystem::path, std::error_code> getConfigPath();

/// Get path to \c walng cache directory
///
/// One of:
///  - $XDG_CACHE_HOME/walng
///  - $HOME/.cache/walng
std::expected<std::filesystem::path, std::error_code> getCachePath();

/// Make temporary file path
std::expected<std::filesystem::path, std::error_code> makeTempFilePath();

/// Expand @c ~ to $HOME
std::expected<void, std::error_code> expandTilda(std::filesystem::path& path);

} // namespace walng
