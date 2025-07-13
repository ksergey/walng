// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

module;

#include <expected>
#include <filesystem>
#include <format>
#include <string>

#include <yaml-cpp/yaml.h>

import walng.utils;

module walng.config;

namespace walng {

auto load_config_from_yaml_file(std::filesystem::path const& path) -> std::expected<config, std::string> {
  try {
    auto const yaml = YAML::LoadFile(path.c_str());

    config result;
    result.shell_exec_cmd = yaml["config"]["shell"].as<std::string>("/bin/sh -c '{}'");
    if (auto const found = result.shell_exec_cmd.find("{}"); found == result.shell_exec_cmd.npos) {
      return std::unexpected("shell string missed command placeolder ({})");
    }

    auto const& yaml_items = yaml["items"];

    auto const items_count = yaml_items.size();
    result.items.reserve(items_count);

    for (std::size_t i = 0; i < items_count; ++i) {
      auto const& yaml_item = yaml_items[i];

      auto& template_item = result.items.emplace_back();
      template_item.name = yaml_item["name"].as<std::string>();

      template_item.template_path = yaml_item["template"].as<std::string>();
      if (auto const rc = expand_tilda(template_item.template_path); !rc) {
        return std::unexpected(
            std::format("failed to expand tilda in template value of item '{}'", template_item.name));
      }

      template_item.target_path = yaml_item["target"].as<std::string>();
      if (auto const rc = expand_tilda(template_item.target_path); !rc) {
        return std::unexpected(std::format("failed to expand tilda in target value of item '{}'", template_item.name));
      }

      template_item.hook_cmd = yaml_item["hook"].as<std::string>("");
    }

    return {std::move(result)};
  } catch (YAML::Exception const& e) {
    return std::unexpected(e.what());
  }
}

} // namespace walng
