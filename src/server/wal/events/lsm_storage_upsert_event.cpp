#include "lsm_storage_upsert_event.hpp"

#include <spdlog/spdlog.h>

#include <database/database.hpp>

namespace structuredb::server::wal {

LsmStorageUpsertEvent::LsmStorageUpsertEvent(
      const std::string& storage_id,
      const lsm::Sequence seq_no,
      const std::string& key,
      const std::string& value
)
  : storage_id_{storage_id}, seq_no_{seq_no}, key_{key}, value_{value}
{}

Awaitable<Event::Ptr> LsmStorageUpsertEvent::Parse(sdb::Reader& reader) {
  auto result = std::make_unique<LsmStorageUpsertEvent>(
    co_await reader.ReadString(),
    co_await reader.ReadInt(),
    co_await reader.ReadString(),
    co_await reader.ReadString()
  );
  co_return result;
}

EventType LsmStorageUpsertEvent::GetType() const {
  return EventType::kLsmStorageUpsert;
}

Awaitable<void> LsmStorageUpsertEvent::Flush(sdb::Writer& writer) {
  co_await writer.WriteString(storage_id_);
  co_await writer.WriteInt(seq_no_);
  co_await writer.WriteString(key_);
  co_await writer.WriteString(value_);
}

Awaitable<void> LsmStorageUpsertEvent::Apply(database::Database& db) {
  SPDLOG_DEBUG("Got upsert event for storage = {}, key = {}, value = {}", storage_id_, key_, value_);
  auto table = db.GetStorageForRecover(storage_id_);
  if (!table) {
    SPDLOG_ERROR("Got nullptr after GetTable during recovery");
    co_return;
  }
  co_await table->RecoverFromLog(seq_no_, key_, value_);
}

}
