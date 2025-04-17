// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: AGPL-3.0

#include "Template.h"

#include <print>

namespace walng {

void Template::render(inja::json const& theme, std::filesystem::path const& templatePath) {
  inja::Environment env;
  env.add_callback("hex", 1, [](inja::Arguments const& args) {
    auto const color = args.at(0)->get<std::string_view>();
    return std::string(color) + "--";
  });

  auto result = env.render_file(templatePath.c_str(), theme);

  std::print("Result:\n{}\n", result);
}

} // namespace walng
