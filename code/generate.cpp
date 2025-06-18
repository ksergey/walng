// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include "generate.h"

#include <cstdio>
#include <format>
#include <print>

#include <inja/inja.hpp>

#include "colors.h"
#include "config.h"
#include "theme.h"
#include "utils.h"

namespace walng {
namespace {

void configure(inja::Environment& env) {
  env.set_trim_blocks(true);
  env.set_lstrip_blocks(true);

  auto const parseColorOrThrow = [](std::string_view colorStr) -> Color {
    auto const color = walng::parseColorFromHexStr(colorStr);
    if (!color) {
      throw std::runtime_error(std::format("'{}' is not valid color", colorStr));
    }
    return *color;
  };

  env.add_callback("hex", 1, [&](inja::Arguments const& args) {
    auto const color = parseColorOrThrow(args.at(0)->get<std::string>()).asRGB();
    return std::format("{:02x}{:02x}{:02x}", color.r, color.g, color.b);
  });

  env.add_callback("rgb", 1, [&](inja::Arguments const& args) {
    auto const color = parseColorOrThrow(args.at(0)->get<std::string>()).asRGB();
    return std::format("{}, {}, {}", color.r, color.g, color.b);
  });
};

std::string makeSystemExecCommand(std::string const& shell, std::string const& command) {
  auto const found = shell.find("{}");
  if (found == shell.npos) {
    throw std::runtime_error("something wrong with shell");
  }
  return std::string().append(shell, 0, found).append(command).append(shell, found + 2);
}

} // namespace

void generate(std::filesystem::path const& configPath, std::filesystem::path const& themePath) {
  auto const config = loadConfigFromYAMLFile(configPath);
  auto jsonThemeRoot = loadBaseXXThemeFromYAMLFile(themePath);

  inja::Environment env;
  configure(env);

  auto tempPath = makeTempFilePath();
  if (!tempPath.has_value()) {
    throw tempPath.error();
  }

  for (auto const& item : config.items) {
    try {
      env.write(item.templatePath, jsonThemeRoot, tempPath.value());
      if (exists(item.targetPath)) {
        remove(item.targetPath);
      }
      std::filesystem::copy_file(tempPath.value(), item.targetPath);
      remove(tempPath.value());

      if (!item.hook.empty()) {
        auto const command = makeSystemExecCommand(config.shell, item.hook);
        [[maybe_unused]] auto const rc = ::system(command.c_str());
      }
    } catch (std::exception const& e) {
      std::print(stderr, "failed to process item '{}': {}\n", item.name, e.what());
    }
  }
}

} // namespace walng
