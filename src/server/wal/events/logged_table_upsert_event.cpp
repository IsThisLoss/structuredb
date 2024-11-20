#include "logged_table_upsert_event.hpp"

#include <iostream>

#include <database/database.hpp>

namespace structuredb::server::wal {

LoggedTableUpsertEvent::LoggedTableUpsertEvent(
      const std::string& table_name,
      const lsm::Sequence seq_no,
      const std::string& key,
      const std::string& value
)
  : table_name_{table_name}, seq_no_{seq_no}, key_{key}, value_{value}
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
  co_await writer.WriteString(table_name_);
  co_await writer.WriteInt(seq_no_);
  co_await writer.WriteString(key_);
  co_await writer.WriteString(value_);
}

Awaitable<void> LoggedTableUpsertEvent::Apply(database::Database& db) {
  std::cerr << "Recover: " << table_name_ << " " << key_ << " " << value_ << std::endl;
  if (table_name_ == "sys_transactions") {
    auto table = db.GetTxTable();
    co_await table->RecoverFromLog(seq_no_, key_, value_);
    co_return;
  }
  auto table = db.GetTableForRecover(table_name_);
  if (!table) {
    std::cerr << "Got nullptr after GetTable\n";
    co_return;
  }
  std::cerr << "Recover: " << key_ << " " << value_ << std::endl;
  co_await table->RecoverFromLog(seq_no_, key_, value_);
}

}
