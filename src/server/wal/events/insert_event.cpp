#include "insert_event.hpp"

#include <iostream>

namespace structuredb::server::wal {

InsertEvent::InsertEvent(const std::string& key, const std::string& value)
  : key_{key}, value_{value} {}

Awaitable<Event::Ptr> InsertEvent::Parse(sdb::Reader& reader) {
  auto result = std::make_unique<InsertEvent>(
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
  co_await writer.WriteString(key_);
  co_await writer.WriteString(value_);
}

Awaitable<void> InsertEvent::Apply(lsm::Lsm& lsm) {
  co_await lsm.Put(key_, value_);
}

}
