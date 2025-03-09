#include "compaction.hpp"

#include <database/database.hpp>

namespace structuredb::server::database {

Compaction::Compaction(Database& database, const std::chrono::milliseconds interval)
  : database_{database}
  , interval_{interval}
{}

std::chrono::milliseconds Compaction::GetInterval() const {
  return interval_;
}

Awaitable<void> Compaction::Step() {
  SPDLOG_INFO("Compacting database");

  auto session = co_await database_.StartSession();
  auto sys_tables = co_await session.GetTable("sys_tables");
  if (!sys_tables) {
    SPDLOG_ERROR("Cannot get sys_tables during compaction");
    co_return;
  }

  // scan all tables
  auto iter = co_await sys_tables->Scan(std::nullopt, std::nullopt);

  while (iter->HasMore()) {
      auto table_info = co_await iter->Next();
      auto table = co_await session.GetTable(table_info.key);
      if (!table) {
        SPDLOG_ERROR("Cannot get table {} during compaction", table_info.key);
        continue;
      }

      SPDLOG_INFO("Compacting table {}", table_info.key);
      co_await table->Compact();
      SPDLOG_INFO("Compacted table {}", table_info.key);
  }

  SPDLOG_INFO("Database compaction finished");
}

}
