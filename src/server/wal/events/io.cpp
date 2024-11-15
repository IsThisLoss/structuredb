#include "io.hpp"

#include "insert_event.hpp"

namespace structuredb::server::wal {

namespace {

using Parser = std::function<Awaitable<Event::Ptr>(sdb::Reader&)>;

std::unordered_map<EventType, Parser> kParsers{
  {EventType::kInsert, InsertEvent::Parse},
};

}

Awaitable<Event::Ptr> ParseEvent(sdb::Reader& reader) {
  const auto type = static_cast<EventType>(co_await reader.ReadInt());
  auto event = co_await kParsers.at(type)(reader);
  co_return event;
}

Awaitable<void> FlushEvent(sdb::Writer& writer, const Event::Ptr& event) {
  co_await writer.WriteInt(static_cast<int64_t>(event->GetType()));
  co_await event->Flush(writer);
}

}
