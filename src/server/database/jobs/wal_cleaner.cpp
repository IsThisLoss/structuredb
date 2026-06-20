#include "wal_cleaner.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <database/database.hpp>
#include <wal/cleaner.hpp>
#include <spdlog/spdlog.h>

namespace structuredb::server::database {

WalCleaner::WalCleaner(
    io::Manager& io_manager,
    Database& database,
    std::string wal_dir_path,
    std::chrono::milliseconds interval,
    replication::FollowerRegistry::Ptr followers
)
  : io_manager_{io_manager}
  , database_{database}
  , wal_dir_path_{std::move(wal_dir_path)}
  , interval_{interval}
  , followers_{std::move(followers)}
{}

std::chrono::milliseconds WalCleaner::GetInterval() const {
  return interval_;
}

Awaitable<void> WalCleaner::Step() {
  SPDLOG_INFO("Cleaning WAL files");

  std::optional<int64_t> retain_from_segment;
  if (followers_) {
    retain_from_segment = followers_->MinRetainedSegment();
  }

  co_await wal::Clean(io_manager_, wal_dir_path_, database_, retain_from_segment);

  SPDLOG_INFO("WAL cleaning finished");
}
  
}
