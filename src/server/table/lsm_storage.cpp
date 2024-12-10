#include "lsm_storage.hpp"

#include <spdlog/spdlog.h>

#include <wal/events/lsm_storage_upsert_event.hpp>

namespace structuredb::server::table {

LsmStorage::LsmStorage(io::Manager& io_manager, const std::string& base_dir, const std::string& id)
  : lsm_{io_manager, base_dir}, id_{id}
{}

Awaitable<void> LsmStorage::Init() {
  co_await lsm_.Init();
}

void LsmStorage::StartLogInto(wal::Writer::Ptr wal_writer) {
  wal_writer_ = std::move(wal_writer);
  SPDLOG_INFO("Start wal for table {}", id_);
}

Awaitable<void> LsmStorage::RecoverFromLog(const lsm::Sequence seq_no, const std::string& key, const std::string& value) {
  const bool is_restored = co_await lsm_.Put(seq_no, key, value);
  SPDLOG_DEBUG("Recover record: seq_no = {}, key = {}, value = {}, status", seq_no, key, value, is_restored ? "APPLIED" : "SKIPPED");
}

Awaitable<void> LsmStorage::Upsert(const std::string& key, const std::string& value) {
  const auto seq_no = co_await lsm_.Put(key, value);

  if (wal_writer_) {
    co_await wal_writer_->Write(std::make_unique<wal::LsmStorageUpsertEvent>(id_, seq_no, key, value));
  }
}

Awaitable<std::optional<std::string>> LsmStorage::Get(const std::string& key) {
  SPDLOG_DEBUG("Get table {} for key {}", id_, key);
  const auto result = co_await lsm_.Get(key);
  SPDLOG_DEBUG("Get table {} for key {}, result {}", id_, key, result.value_or("<null>"));
  co_return result;
}

Awaitable<void> LsmStorage::Scan(const std::string& key, const lsm::RecordConsumer& consume) {
  SPDLOG_DEBUG("Scan table {} for key {}", id_, key);
  co_await lsm_.Scan(key, consume);
}

Awaitable<lsm::Iterator::Ptr> LsmStorage::Iter() {
  co_return std::make_shared<lsm::LsmIterator>(co_await lsm::LsmIterator::Create(lsm_));
}

}
