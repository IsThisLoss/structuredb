#include "flush.hpp"

#include <chrono>
#include <database/database.hpp>
#include <spdlog/spdlog.h>

namespace structuredb::server::database {

Flush::Flush(Database& database, const std::chrono::milliseconds interval)
  : database_{database}
  , interval_{interval}
{}

std::chrono::milliseconds Flush::GetInterval() const {
  return interval_;
}

Awaitable<void> Flush::Step() {
  SPDLOG_DEBUG("Flushing database");
  co_await database_.Flush();
  SPDLOG_DEBUG("Database flush finished");
}

}
