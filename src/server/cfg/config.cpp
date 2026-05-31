#include "config.hpp"

#include <chrono>
#include <string>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace structuredb::server::cfg {

namespace {

Logger ParseLogger(const YAML::Node& node){ 
  Logger result{};

  if (node["level"]) {
    auto str_lvl = node["level"].as<std::string>();
    result.level = spdlog::level::from_str(str_lvl);
    if (result.level == spdlog::level::off) {
      throw std::runtime_error{"Invalid log level value: " + str_lvl};
    }
  }

  if (node["console"]) {
    result.console = node["console"].as<bool>();
  }

  if (node["file"]) {
    result.file = node["file"].as<std::string>();
  }

  return result;
}

Compaction ParseCompaction(const YAML::Node& node) {
  Compaction result{};

  if (node["interval"]) {
    result.interval = std::chrono::milliseconds{node["interval"].as<int>()};
  }

  return result;
}

WalClean ParseWalClean(const YAML::Node& node) {
  WalClean result{};

  if (node["interval"]) {
    result.interval = std::chrono::milliseconds{node["interval"].as<int>()};
  }

  return result;
}

Wal ParseWal(const YAML::Node& node) {
  Wal result{};

  if (node["clean"]) {
    result.clean = ParseWalClean(node["clean"]);
  }

  return result;
}

}

Config Parse(const std::string& cfg_path) {
  Config result{};

  auto yaml = YAML::LoadFile(cfg_path);

  if (yaml["root"]) {
    result.root = yaml["root"].as<std::string>();
  }

  if (yaml["logger"]) {
    result.logger = ParseLogger(yaml["logger"]);
  }

  if (yaml["port"]) {
    result.port = yaml["port"].as<int>();
  }

  if (yaml["compaction"]) {
    result.compaction = ParseCompaction(yaml["compaction"]);
  }
  if (yaml["wal"]) {
    result.wal = ParseWal(yaml["wal"]);
  }

  return result;
}

}
