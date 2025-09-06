#include "io.hpp"

#include <spdlog/spdlog.h>

#include "lsm_storage_upsert_event.hpp"

namespace structuredb::server::wal {

namespace {

using Parser = std::function<Event::Ptr(sdb::Reader&)>;

const std::unordered_map<EventType, Parser> kParsers{
  {EventType::kLsmStorageUpsert, LsmStorageUpsertEvent::Parse},
};

}

Event::Ptr ParseEvent(sdb::Reader& reader) {
  const auto type = static_cast<EventType>(reader.ReadInt());
  SPDLOG_DEBUG("Parsing event of type {}", static_cast<int>(type));
  auto event = kParsers.at(type)(reader);
  return event;
}

void FlushEvent(sdb::Writer& writer, const Event::Ptr& event) {
  SPDLOG_DEBUG("Flushing event of type {}", static_cast<int>(event->GetType()));
  writer.WriteInt(static_cast<int64_t>(event->GetType()));
  event->Flush(writer);
}

}
