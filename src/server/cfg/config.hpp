#pragma once

#include <string>

#include <spdlog/spdlog.h>

namespace structuredb::server::cfg {

struct Logger {
  spdlog::level::level_enum level{spdlog::level::info};
  bool console{false};
  std::optional<std::string> file{std::nullopt};
};

struct Config {
  int port{50051};
  std::string root{"/tmp/structuredb"};
  Logger logger{};
};

Config Parse(const std::string& cfg_path);

}
