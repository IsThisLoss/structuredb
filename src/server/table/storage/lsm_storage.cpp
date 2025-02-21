#include "lsm_storage.hpp"

#include <spdlog/spdlog.h>

#include <wal/events/lsm_storage_upsert_event.hpp>

namespace structuredb::server::table::storage {

namespace {

class LsmStorageIterator : public Iterator {
public:
  LsmStorageIterator(lsm::Iterator::Ptr iterator)
    : iterator_{std::move(iterator)}
  {
    assert(iterator_);
  }

  bool HasMore() override {
    return iterator_->HasMore();
  }

  Awaitable<Row> Next() override {
    auto record = co_await iterator_->Next();
    co_return Row{
      .key = std::move(record.key),
      .value = std::move(record.value),
    };
  }

private:
  lsm::Iterator::Ptr iterator_;
};

}

LsmStorage::LsmStorage(io::Manager& io_manager, std::string base_dir, std::string id)
  : lsm_{io_manager, std::move(base_dir)}, id_{std::move(id)}
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

Awaitable<void> LsmStorage::Upsert(const Row& row) {
  const auto seq_no = co_await lsm_.Put(row.key, row.value);

  if (wal_writer_) {
    co_await wal_writer_->Write(std::make_unique<wal::LsmStorageUpsertEvent>(id_, seq_no, row.key, row.value));
  }
}

Awaitable<Iterator::Ptr> LsmStorage::Scan(const std::string& key) {
  SPDLOG_DEBUG("Scan storage {} for key {}", id_, key);
  auto result = co_await lsm_.Scan(key);
  co_return std::make_shared<LsmStorageIterator>(std::move(result));
}

Awaitable<Iterator::Ptr> LsmStorage::Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound) {
  lsm::ScanRange range{
      .lower_bound = lower_bound,
      .upper_bound = upper_bound,
  };
  auto result = co_await lsm_.Scan(range);
  co_return std::make_shared<LsmStorageIterator>(std::move(result));
}

Awaitable<void> LsmStorage::Compact(lsm::CompactionStrategy::Ptr strategy) {
  co_await lsm_.Compact(std::move(strategy));
}

}
