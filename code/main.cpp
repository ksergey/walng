// Copyright (c) Sergey Kovalevich <inndie@gmail.com>
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <print>
#include <span>

#include <cxxopts.hpp>
#include <inja/inja.hpp>
#include <yaml-cpp/yaml.h>

namespace walng {

std::filesystem::path const& getHomePath() {
  static std::filesystem::path homePath = [] {
    if (auto const result = ::secure_getenv("HOME"); result) {
      return std::filesystem::path(result);
    }
    throw std::runtime_error("$HOME environment not set");
  }();
  return homePath;
}

std::filesystem::path const& getConfigPathBase() {
  static std::filesystem::path configPathBase = [] {
    if (auto const result = secure_getenv("XDG_CONFIG_HOME"); result) {
      return std::filesystem::path(result) / "walng";
    }
    return getHomePath() / ".config" / "walng";
  }();
  return configPathBase;
}

struct Base16 {
  /// Base16 color names
  static constexpr std::array colors{
      "base00",
      "base01",
      "base02",
      "base03",
      "base04",
      "base05",
      "base06",
      "base07",
      "base08",
      "base09",
      "base0A",
      "base0B",
      "base0C",
      "base0D",
      "base0E",
      "base0F",
  };
};

struct Base24 {
  /// Base24 color names
  static constexpr std::array colors{
      "base00",
      "base01",
      "base02",
      "base03",
      "base04",
      "base05",
      "base06",
      "base07",
      "base08",
      "base09",
      "base0A",
      "base0B",
      "base0C",
      "base0D",
      "base0E",
      "base0F",
      "base10",
      "base11",
      "base12",
      "base13",
      "base14",
      "base15",
      "base16",
      "base17",
  };
};

struct RGB {
  std::uint8_t r; ///< Red [0..255]
  std::uint8_t g; ///< Green [0..255]
  std::uint8_t b; ///< Blue [0..255]
};

struct RGBA {
  std::uint8_t r; ///< Red [0..255]
  std::uint8_t g; ///< Green [0..255]
  std::uint8_t b; ///< Blue [0..255]
  std::uint8_t a; ///< Alpha [0..255]
};

/// parse RGB color from hex string
[[nodiscard]] std::optional<RGB> parseRGBFromHexStr(char const* str) noexcept {
  char* endptr;

  auto const value = ::strtol(str + 1, &endptr, 16);
  if ((errno == EINVAL) || (errno == ERANGE) || (str[0] != '#') || ((endptr - str) != 7)) {
    return std::nullopt;
  }

  RGB color;
  color.r = static_cast<std::uint8_t>((value & 0x00FF0000) >> 16);
  color.g = static_cast<std::uint8_t>((value & 0x0000FF00) >> 8);
  color.b = static_cast<std::uint8_t>(value & 0x000000FF);

  return color;
}

/// \overload
[[nodiscard]] std::optional<RGB> parseRGBFromHexStr(std::string const& str) noexcept {
  return parseRGBFromHexStr(str.c_str());
}

/// parse RGBA color from hex string
[[nodiscard]] std::optional<RGBA> parseRGBAFromHexStr(char const* str) noexcept {
  char* endptr;

  auto const value = ::strtol(str + 1, &endptr, 16);
  auto const length = static_cast<std::size_t>(endptr - str);
  if ((errno == EINVAL) || (errno == ERANGE) || (str[0] != '#') || ((length != 7) && (length != 9))) {
    return std::nullopt;
  }

  RGBA color;

  if (length == 9) {
    color.r = static_cast<std::uint8_t>((value & 0xFF000000) >> 24);
    color.g = static_cast<std::uint8_t>((value & 0x00FF0000) >> 16);
    color.b = static_cast<std::uint8_t>((value & 0x0000FF00) >> 8);
    color.a = static_cast<std::uint8_t>(value & 0x000000FF);
  } else {
    color.r = static_cast<std::uint8_t>((value & 0x00FF0000) >> 16);
    color.g = static_cast<std::uint8_t>((value & 0x0000FF00) >> 8);
    color.b = static_cast<std::uint8_t>(value & 0x000000FF);
    color.a = 255;
  }

  return color;
}

/// \overload
[[nodiscard]] std::optional<RGBA> parseRGBAFromHexStr(std::string const& str) noexcept {
  return parseRGBAFromHexStr(str.c_str());
}

/// load base16 or base24 theme from yaml file
[[nodiscard]] inja::json loadBaseXXThemeFromYAMLFile(std::filesystem::path const& path) {
  auto const yamlRootNode = YAML::LoadFile(path.c_str());
  auto jsonRootNode = inja::json::object();

  for (auto const entry : {"name", "author", "variant"}) {
    jsonRootNode[entry] = yamlRootNode[entry].as<std::string>();
  }

  std::span<char const* const> paletteColors;
  auto const system = yamlRootNode["system"].as<std::string>();
  if (system == "base16") {
    paletteColors = std::span(Base16::colors);
  } else if (system == "base24") {
    paletteColors = std::span(Base24::colors);
  } else {
    throw std::runtime_error(std::format("unknown palette color system ('{}')", system));
  }

  auto const yamlPaletteNode = yamlRootNode["palette"];
  auto jsonPaletteNode = inja::json::object();
  for (auto const color : paletteColors) {
    auto const hex = yamlPaletteNode[color].as<std::string>();
    if (auto const value = parseRGBFromHexStr(hex); !value) {
      throw std::runtime_error(std::format("invalid hex color value '{}' ('{}')", color, hex));
    }
    // jsonRootNode[color] = hex;
    jsonPaletteNode[color] = hex;
  }
  jsonRootNode["palette"] = jsonPaletteNode;

  return jsonRootNode;
}

struct Config {
  struct Item {
    std::string name;
    std::filesystem::path templatePath;
    std::filesystem::path targetPath;
    std::string hook;
  };

  std::string shell;
  std::vector<Item> items;
};

/// Load config from yaml file
[[nodiscard]] Config loadConfigFromYAMLFile(std::filesystem::path configPath) {
  auto const yamlRootNode = YAML::LoadFile(configPath.c_str());

  Config config;
  config.shell = yamlRootNode["config"]["shell"].as<std::string>("/bin/sh -c '{}'");

  auto const yamlItemsNode = yamlRootNode["items"];
  auto const itemsCount = yamlItemsNode.size();
  config.items.reserve(itemsCount);

  for (std::size_t i = 0; i < itemsCount; ++i) {
    auto const& yamlItemNode = yamlItemsNode[i];

    auto& item = config.items.emplace_back();
    item.name = yamlItemNode["name"].as<std::string>();
    if (auto const templatePath = yamlItemNode["template"].as<std::string>(); templatePath.starts_with("~/")) {
      item.templatePath = getHomePath() / templatePath.substr(2);
    } else {
      item.templatePath = templatePath;
    }
    if (auto const targetPath = yamlItemNode["target"].as<std::string>(); targetPath.starts_with("~/")) {
      item.targetPath = getHomePath() / targetPath.substr(2);
    } else {
      item.targetPath = targetPath;
    }
    item.hook = yamlItemNode["hook"].as<std::string>("");
  }

  return config;
}

} // namespace walng

int main(int argc, char* argv[]) {
  try {
    cxxopts::Options options("walng", "template coloscheme generator");

    // clang-format off
    options.add_options()
      ("theme", "path to theme file", cxxopts::value<std::string>(), "PATH")
      ("config", "path to config file", cxxopts::value<std::string>(), "PATH")
      ("help", "print help and exit")
    ;
    // clang-format on

    auto const result = options.parse(argc, argv);
    if (result.count("help")) {
      std::print(stdout, "{}\n", options.help());
      return EXIT_FAILURE;
    }

    std::filesystem::path const configPath = [&] {
      if (result.count("config")) {
        return std::filesystem::path(result["config"].as<std::string>());
      }
      return walng::getConfigPathBase() / "config.yaml";
    }();

    auto const config = walng::loadConfigFromYAMLFile(configPath);

    if (result.count("theme") == 0) {
      throw std::runtime_error("argument '--theme' is required");
    }
    std::filesystem::path const themePath = result["theme"].as<std::string>();

    auto const themeData = walng::loadBaseXXThemeFromYAMLFile(themePath);
    // std::print("{}\n", themeData.dump(2));

    inja::Environment env;

    env.add_callback("rgb", 1, [](inja::Arguments const& args) {
      auto const colorStr = args.at(0)->get<std::string>();
      auto const colorRGBA = walng::parseRGBAFromHexStr(colorStr);
      if (!colorRGBA) {
        throw std::runtime_error(std::format("'{}' not valid color", colorStr));
      }
      return std::format("rgb({}, {}, {})", colorRGBA->r, colorRGBA->g, colorRGBA->b);
    });

    env.add_callback("rgba", 1, [](inja::Arguments const& args) {
      auto const colorStr = args.at(0)->get<std::string>();
      auto const colorRGBA = walng::parseRGBAFromHexStr(colorStr);
      if (!colorRGBA) {
        throw std::runtime_error(std::format("'{}' not valid color", colorStr));
      }
      return std::format(
          "rgba({}, {}, {}, {:.1f})", colorRGBA->r, colorRGBA->g, colorRGBA->b, float(colorRGBA->a) / 255.0);
    });

    env.add_callback("alpha", 2, [](inja::Arguments const& args) {
      auto const colorStr = args.at(0)->get<std::string>();
      auto const alphaFloat = std::clamp<float>(args.at(1)->get<float>(), 0.0, 1.0);
      auto const colorRGBA = walng::parseRGBAFromHexStr(colorStr);
      if (!colorRGBA) {
        throw std::runtime_error(std::format("'{}' not valid color", colorStr));
      }
      return std::format("#{:02x}{:02x}{:02x}{:02x}", colorRGBA->r, colorRGBA->g, colorRGBA->b,
          static_cast<std::uint8_t>(alphaFloat * 255));
    });

    env.add_callback("hex_stripped", 1, [](inja::Arguments const& args) {
      auto const colorStr = args.at(0)->get<std::string>();
      auto const colorRGBA = walng::parseRGBAFromHexStr(colorStr);
      if (!colorRGBA) {
        throw std::runtime_error(std::format("'{}' not valid color", colorStr));
      }
      return std::format("{:02x}{:02x}{:02x}", colorRGBA->r, colorRGBA->g, colorRGBA->b);
    });

    for (auto const& item : config.items) {
      try {
        env.write(item.templatePath, themeData, item.targetPath);
        if (!item.hook.empty()) {
          // TODO: trigger hook
        }
      } catch (std::exception const& e) {
        std::print(stderr, "failed to process item '{}': {}\n", item.name, e.what());
      }
    }
  } catch (std::exception const& e) {
    std::print(stderr, "Error: {}\n", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
