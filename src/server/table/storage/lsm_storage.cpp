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

  lsm::Sequence GetLastSeqNo() {
    return last_seq_no_;
  }

  Awaitable<Row> Next() override {
    auto record = co_await iterator_->Next();
    last_seq_no_ = record.seq_no;
    co_return Row{
      .key = std::move(record.key),
      .value = std::move(record.value),
    };
  }

private:
  lsm::Iterator::Ptr iterator_;
  lsm::Sequence last_seq_no_{0};
};

class LsmOutputIterator : public OutputIterator {
public:
  explicit LsmOutputIterator(
      std::shared_ptr<LsmStorageIterator> input_iterator,
      lsm::disk::SSTableBuilder& ss_table_builder
  )
    : input_iterator_{std::move(input_iterator)}
    , ss_table_builder_{ss_table_builder}
  {
    assert(input_iterator_);
  }

  Awaitable<void> Write(Row row) override {
    lsm::Record record{
      .key = std::move(row.key),
      .seq_no = input_iterator_->GetLastSeqNo(),
      .value = std::move(row.value),
    };
    co_await ss_table_builder_.Add(record);
  }

private:
  std::shared_ptr<LsmStorageIterator> input_iterator_;
  lsm::disk::SSTableBuilder& ss_table_builder_;
};

class LsmStorageCompactStrategy : public lsm::CompactionStrategy {
public:
  explicit LsmStorageCompactStrategy(table::storage::CompactionStrategy::Ptr strategy)
    : strategy_{std::move(strategy)}
  {}

  Awaitable<void> CompactRecords(lsm::Iterator::Ptr records, lsm::disk::SSTableBuilder& ss_table_builder) override {
    auto input = std::make_shared<LsmStorageIterator>(std::move(records));
    auto output = std::make_shared<LsmOutputIterator>(input, ss_table_builder);
    co_await strategy_->CompactRows(std::move(input), output);
  }

private:
  table::storage::CompactionStrategy::Ptr strategy_;
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

Awaitable<void> LsmStorage::Compact(CompactionStrategy::Ptr strategy) { 
  co_await lsm_.Compact(std::make_shared<LsmStorageCompactStrategy>(std::move(strategy)));
}

int LsmStorage::CountSSTables() const {
  return lsm_.CountSSTables();
}

}
