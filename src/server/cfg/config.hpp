#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include <spdlog/spdlog.h>

namespace structuredb::server::cfg {

const int kDefaultPort{50051};

struct Logger {
  spdlog::level::level_enum level{spdlog::level::info};
  bool console{false};
  std::optional<std::string> file{std::nullopt};
};

struct WalClean {
  std::chrono::milliseconds interval{std::chrono::minutes(1)};
};

struct Wal {
  WalClean clean{};
};

struct Compaction {
  std::chrono::milliseconds interval{std::chrono::minutes(1)};
};

struct Flush {
  std::chrono::milliseconds interval{std::chrono::seconds(1)};
};

struct Lsm {
  size_t max_records_in_mem_table{10000};
  size_t max_ro_mem_tables{1};
  int64_t page_size{4096};
  size_t page_cache_capacity{1024};
};

enum class ReplicationRole {
  kLeader,
  kFollower,
};

struct Replication {
  ReplicationRole role{ReplicationRole::kLeader};
  // address (host:port) of the leader to follow; required when role == kFollower
  std::string leader_address{};
  // how often the leader re-scans the WAL for new pages to ship
  std::chrono::milliseconds poll_interval{std::chrono::milliseconds{200}};
};

struct Config {
  int port{kDefaultPort};
  std::string root{"/tmp/structuredb"};
  Logger logger{};
  Compaction compaction{};
  Flush flush{};
  Wal wal{};
  Lsm lsm{};
  Replication replication{};
};

Config Parse(const std::string& cfg_path);

}
