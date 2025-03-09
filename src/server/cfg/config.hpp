#pragma once

#include <string>

#include <spdlog/spdlog.h>

namespace structuredb::server::cfg {

const int kDefaultPort{50051};

struct Logger {
  spdlog::level::level_enum level{spdlog::level::info};
  bool console{false};
  std::optional<std::string> file{std::nullopt};
};

struct Compaction {
  std::chrono::milliseconds interval{std::chrono::minutes(1)};
};

struct Config {
  int port{kDefaultPort};
  std::string root{"/tmp/structuredb"};
  Logger logger{};
  Compaction compaction{};
};

Config Parse(const std::string& cfg_path);

}
