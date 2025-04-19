// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace walng {

/// walng configuration
struct Config {
  /// Template item
  struct Item {
    std::string name;
    std::filesystem::path templatePath;
    std::filesystem::path targetPath;
    std::string hook;
  };

  /// Shell str for executing hooks
  std::string shell;

  /// Templates to process
  std::vector<Item> items;
};

/// Load configuration from yaml file
/// @throw std::runtime_error on error
Config loadConfigFromYAMLFile(std::filesystem::path const& configPath);

} // namespace walng
