// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "config.h"

#include <format>

#include <yaml-cpp/yaml.h>

#include "utils.h"

namespace walng {

Config loadConfigFromYAMLFile(std::filesystem::path const& configPath) {
  auto const yamlRootNode = YAML::LoadFile(configPath.c_str());

  Config config;
  config.shell = yamlRootNode["config"]["shell"].as<std::string>("/bin/sh -c '{}'");
  if (auto const found = config.shell.find("{}"); found == config.shell.npos) {
    throw std::runtime_error(std::format("shell string missed '{{}}'"));
  }

  auto const yamlItemsNode = yamlRootNode["items"];
  auto const itemsCount = yamlItemsNode.size();
  config.items.reserve(itemsCount);

  for (std::size_t i = 0; i < itemsCount; ++i) {
    auto const& yamlItemNode = yamlItemsNode[i];

    auto& item = config.items.emplace_back();
    item.name = yamlItemNode["name"].as<std::string>();

    item.templatePath = yamlItemNode["template"].as<std::string>();
    if (auto const rc = expandTilda(item.templatePath); !rc) {
      throw rc.error();
    }
    if (!exists(item.templatePath)) {
      throw std::runtime_error(
          std::format("template {} not found for item '{}'", item.templatePath.c_str(), item.name));
    }
    item.targetPath = yamlItemNode["target"].as<std::string>();
    if (auto const rc = expandTilda(item.targetPath); !rc) {
      throw rc.error();
    }
    if (!exists(item.targetPath.parent_path())) {
      throw std::runtime_error(std::format(
          "target parent path {} not exists for item '{}'", item.targetPath.parent_path().c_str(), item.name));
    }

    item.hook = yamlItemNode["hook"].as<std::string>("");
  }

  return config;
}

} // namespace walng
