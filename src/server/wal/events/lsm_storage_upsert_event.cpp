#include "lsm_storage_upsert_event.hpp"

#include <spdlog/spdlog.h>

#include <database/database.hpp>

namespace structuredb::server::wal {

LsmStorageUpsertEvent::LsmStorageUpsertEvent(
      std::string storage_id,
      lsm::Sequence seq_no,
      std::string key,
      std::string value
)
  : storage_id_{std::move(storage_id)}, seq_no_{seq_no}, key_{std::move(key)}, value_{std::move(value)}
{}

Event::Ptr LsmStorageUpsertEvent::Parse(sdb::Reader& reader) {
  auto result = std::make_unique<LsmStorageUpsertEvent>(
    reader.ReadString(),
    reader.ReadInt(),
    reader.ReadString(),
    reader.ReadString()
  );
  return result;
}

EventType LsmStorageUpsertEvent::GetType() const {
  return EventType::kLsmStorageUpsert;
}

void LsmStorageUpsertEvent::Flush(sdb::Writer& writer) {
  writer.WriteString(storage_id_);
  writer.WriteInt(seq_no_);
  writer.WriteString(key_);
  writer.WriteString(value_);
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

Awaitable<bool> LsmStorageUpsertEvent::IsPersistent(database::Database& db) {
  auto table = db.GetStorageForRecover(storage_id_);
  if (!table) {
    SPDLOG_ERROR("Got nullptr after GetTable during checking persistence");
    co_return false;
  }
  const bool is_persistent = co_await table->IsPersistent(seq_no_);
  co_return is_persistent;
}

}
