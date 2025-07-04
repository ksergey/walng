// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

module;

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

export module walng.config;

namespace walng {

/// Application templates configuration
export struct application_template {
  /// Entry name
  std::string name;
  /// Path to template file
  std::filesystem::path template_path;
  /// Path to destination file
  std::filesystem::path target_path;
  /// Hook command
  std::string hook_cmd;
};

/// Application config
export struct config {
  /// Shell command to execute hooks
  std::string shell_exec_cmd;
  /// Application templates
  std::vector<application_template> items;
};

export [[nodiscard]] auto load_config_from_yaml_file(std::filesystem::path const& path)
    -> std::expected<config, std::string>;

} // namespace walng
