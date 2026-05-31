#include "wal_cleaner.hpp"

#include <chrono>
#include <string>
#include <database/database.hpp>
#include <wal/cleaner.hpp>
#include <spdlog/spdlog.h>

namespace structuredb::server::database {

WalCleaner::WalCleaner(io::Manager& io_manager, Database& database, std::string wal_dir_path, std::chrono::milliseconds interval)
  : io_manager_{io_manager}
  , database_{database}
  , wal_dir_path_{std::move(wal_dir_path)}
  , interval_{interval}
{}

std::chrono::milliseconds WalCleaner::GetInterval() const {
  return interval_;
}

Awaitable<void> WalCleaner::Step() {
  SPDLOG_INFO("Cleaning WAL files");

  co_await wal::Clean(io_manager_, wal_dir_path_, database_);

  SPDLOG_INFO("WAL cleaning finished");
}
  
}
