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
  std::cerr << "Parsed: " << result->key_ << ' ' << result->value_ << std::endl;
  co_return result;
}

EventType InsertEvent::GetType() const {
  return EventType::kInsert;
}

Awaitable<void> InsertEvent::Flush(sdb::Writer& writer) {
  std::cerr << "Flush: " << key_ << ' ' << value_ << std::endl;
  co_await writer.WriteInt(tx_);
  co_await writer.WriteString(key_);
  co_await writer.WriteString(value_);
}

Awaitable<void> InsertEvent::Apply(database::Database& db) {
  db.SetTx(tx_);
  auto table = db.GetTable();
  co_await table->Upsert(tx_, key_, value_);
}

}
