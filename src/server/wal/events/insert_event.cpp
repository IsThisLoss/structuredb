#include "insert_event.hpp"

#include <iostream>

#include <database/database.hpp>

namespace structuredb::server::wal {

InsertEvent::InsertEvent(int64_t tx, const std::string& key, const std::string& value)
  : tx_{tx}, key_{key}, value_{value} {}

Awaitable<Event::Ptr> InsertEvent::Parse(sdb::Reader& reader) {
  auto result = std::make_unique<InsertEvent>(
    co_await reader.ReadInt(),
    co_await reader.ReadString(),
    co_await reader.ReadString()
  );
  co_return result;
}

EventType InsertEvent::GetType() const {
  return EventType::kInsert;
}

Awaitable<void> InsertEvent::Flush(sdb::Writer& writer) {
  co_await writer.WriteInt(tx_);
  co_await writer.WriteString(key_);
  co_await writer.WriteString(value_);
}

Awaitable<void> InsertEvent::Apply(database::Database& db) {
  if (tx_ < db.GetTransactionStorage().GetPersistedTx()) {
    // skip events that was persisted
    co_return;
  }
  auto table = db.GetTable();
  std::cerr << "Recover: " << key_ << " " << value_ << std::endl;
  co_await table->Upsert(tx_, key_, value_);
  db.GetTransactionStorage().SetMinCommitedTx(tx_);
}

}
