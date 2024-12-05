#include "logged_table_upsert_event.hpp"

#include <spdlog/spdlog.h>

#include <database/database.hpp>

namespace structuredb::server::wal {

LoggedTableUpsertEvent::LoggedTableUpsertEvent(
      const std::string& table_id,
      const lsm::Sequence seq_no,
      const std::string& key,
      const std::string& value
)
  : table_id_{table_id}, seq_no_{seq_no}, key_{key}, value_{value}
{}

Awaitable<Event::Ptr> LoggedTableUpsertEvent::Parse(sdb::Reader& reader) {
  auto result = std::make_unique<LoggedTableUpsertEvent>(
    co_await reader.ReadString(),
    co_await reader.ReadInt(),
    co_await reader.ReadString(),
    co_await reader.ReadString()
  );
  co_return result;
}

EventType LoggedTableUpsertEvent::GetType() const {
  return EventType::kLoggedTableUpsert;
}

Awaitable<void> LoggedTableUpsertEvent::Flush(sdb::Writer& writer) {
  co_await writer.WriteString(table_id_);
  co_await writer.WriteInt(seq_no_);
  co_await writer.WriteString(key_);
  co_await writer.WriteString(value_);
}

Awaitable<void> LoggedTableUpsertEvent::Apply(database::Database& db) {
  SPDLOG_DEBUG("Got upsert event for table = {}, key = {}, value = {}", table_id_, key_, value_);
  auto table = db.GetTableForRecover(table_id_);
  if (!table) {
    SPDLOG_ERROR("Got nullptr after GetTable during recovery");
    co_return;
  }
  co_await table->RecoverFromLog(seq_no_, key_, value_);
}

}
