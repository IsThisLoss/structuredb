#include "insert_event.hpp"

#include <iostream>

#include <database/database.hpp>

namespace structuredb::server::wal {

InsertEvent::InsertEvent(const std::string& key, const lsm::Sequence seq_no, const std::string& value)
  : key_{key}, seq_no_{seq_no}, value_{value} {}

Awaitable<Event::Ptr> InsertEvent::Parse(sdb::Reader& reader) {
  auto result = std::make_unique<InsertEvent>(
    co_await reader.ReadString(),
    co_await reader.ReadInt(),
    co_await reader.ReadString()
  );
  co_return result;
}

EventType InsertEvent::GetType() const {
  return EventType::kInsert;
}

Awaitable<void> InsertEvent::Flush(sdb::Writer& writer) {
  co_await writer.WriteString(key_);
  co_await writer.WriteInt(seq_no_);
  co_await writer.WriteString(value_);
}

Awaitable<void> InsertEvent::Apply(database::Database& db) {
  auto table = db.GetTable();
  std::cerr << "Recover: " << key_ << " " << value_ << std::endl;
  co_await table->RecoverRecord(key_, seq_no_, value_);
}

}
