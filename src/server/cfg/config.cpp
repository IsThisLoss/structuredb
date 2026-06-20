#include "config.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
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

Flush ParseFlush(const YAML::Node& node) {
  Flush result{};

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

Lsm ParseLsm(const YAML::Node& node) {
  Lsm result{};

  if (node["max_records_in_mem_table"]) {
    result.max_records_in_mem_table = node["max_records_in_mem_table"].as<size_t>();
  }
  if (node["max_ro_mem_tables"]) {
    result.max_ro_mem_tables = node["max_ro_mem_tables"].as<size_t>();
  }
  if (node["page_size"]) {
    result.page_size = node["page_size"].as<int64_t>();
  }
  if (node["page_cache_capacity"]) {
    result.page_cache_capacity = node["page_cache_capacity"].as<size_t>();
  }

  return result;
}

Replication ParseReplication(const YAML::Node& node) {
  Replication result{};

  if (node["role"]) {
    const auto role = node["role"].as<std::string>();
    if (role == "leader") {
      result.role = ReplicationRole::kLeader;
    } else if (role == "follower") {
      result.role = ReplicationRole::kFollower;
    } else {
      throw std::runtime_error{"Invalid replication role: " + role};
    }
  }

  if (node["leader_address"]) {
    result.leader_address = node["leader_address"].as<std::string>();
  }

  if (node["poll_interval"]) {
    result.poll_interval = std::chrono::milliseconds{node["poll_interval"].as<int>()};
  }

  if (result.role == ReplicationRole::kFollower && result.leader_address.empty()) {
    throw std::runtime_error{"replication.leader_address is required when role is follower"};
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
  if (yaml["flush"]) {
    result.flush = ParseFlush(yaml["flush"]);
  }
  if (yaml["wal"]) {
    result.wal = ParseWal(yaml["wal"]);
  }
  if (yaml["lsm"]) {
    result.lsm = ParseLsm(yaml["lsm"]);
  }
  if (yaml["replication"]) {
    result.replication = ParseReplication(yaml["replication"]);
  }

  return result;
}

}
