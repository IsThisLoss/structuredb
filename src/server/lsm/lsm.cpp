#include "lsm.hpp"

#include <spdlog/spdlog.h>

#include "iterators/lsm_range_iterator.hpp"
#include "iterators/lsm_key_iterator.hpp"
#include "iterators/merge_iterator.hpp"

namespace structuredb::server::lsm {

Lsm::Lsm(io::Manager& io_manager, std::string base_dir)
  : io_manager_{io_manager}
  , base_dir_{std::move(base_dir)}
{}

Awaitable<void> Lsm::Init() {
  Sequence max_persistent_seq_no{};
  const auto names = co_await io_manager_.ListDirectory(base_dir_);
  for (const auto& name : names) {
    auto file_reader = co_await io_manager_.CreateFileReader(base_dir_ + "/" + name);
    auto ss_table = co_await SSTable::Create(std::move(file_reader));
    max_persistent_seq_no = std::max(max_persistent_seq_no, ss_table.GetMaxSeqNo());
    ss_tables_.push_back(std::move(ss_table));
  }
  next_seq_no_ = max_persistent_seq_no + 1;
  SPDLOG_INFO("LSM ready, ss tables = {}, next_seq_no = {}", ss_tables_.size(), next_seq_no_);
}

Awaitable<Sequence> Lsm::Put(const std::string& key, const std::string& value) {
  const auto seq_no = next_seq_no_++;
  co_await DoPut(seq_no, key, value);
  co_return seq_no;
}

Awaitable<bool> Lsm::Put(const Sequence seq_no, const std::string& key, const std::string& value) {
  // ignore record out of order
  if (seq_no != next_seq_no_) {
    co_return false;
  }
  co_await DoPut(seq_no, key, value);
  next_seq_no_++;
  co_return true;
}

Awaitable<void> Lsm::DoPut(const Sequence seq_no, const std::string& key, const std::string& value) {
  mem_table_.Put(Record{.key = key, .seq_no = seq_no, .value = value});

  if (mem_table_.Size() > kMaxRecordsInMemTable) {
    SPDLOG_INFO("Mem table reached max size, freeze it");
    ro_mem_tables_.push_back(std::move(mem_table_));
    mem_table_ = MemTable{};
  }

  if (ro_mem_tables_.size() > kMaxRoMemTables) {
    SPDLOG_INFO("Ro Mem tables reached max size, flush it");
    const auto file_path = base_dir_ + "/" + std::to_string(ss_tables_.size()) + ".sst.sdb";
    auto ss_table = co_await ro_mem_tables_.front().Flush(io_manager_, file_path);
    ss_tables_.push_back(std::move(ss_table));
    ro_mem_tables_.erase(ro_mem_tables_.begin());
  }
}

Awaitable<std::optional<std::string>> Lsm::Get(const std::string& key) {
  std::optional<std::string> result;
  const auto iterator = co_await Scan(key);
  while (iterator->HasMore()) {
    auto record = co_await iterator->Next();
    result = record.value;
    break;
  }
  co_return result;
}

Awaitable<Iterator::Ptr> Lsm::Scan(const std::string& key) {
  SPDLOG_INFO("LSM scan: {} {} {}", mem_table_.Size(), ro_mem_tables_.size(), ss_tables_.size());
  auto iterator = co_await LsmKeyIterator::Create(*this, key);
  co_return std::make_shared<LsmKeyIterator>(std::move(iterator));
}

Awaitable<Iterator::Ptr> Lsm::Scan(const ScanRange& range) {
  auto iterator = co_await LsmRangeIterator::Create(*this, range);
  co_return std::make_shared<LsmRangeIterator>(std::move(iterator));
}

Awaitable<void> Lsm::Compact(CompactionStrategy::Ptr strategy) {
  SPDLOG_INFO("Compaction started");

  constexpr static const int64_t kPageSize = 512;

  std::vector<Iterator::Ptr> iterators;
  iterators.reserve(ss_tables_.size());
  for (auto& ss_table : ss_tables_) {
    iterators.push_back(co_await ss_table.Scan(ScanRange::FullScan()));
  }
  auto merge_iterator = std::make_shared<MergeIterator>(co_await MergeIterator::Create(std::move(iterators)));

  const auto file_path = base_dir_ + "/" + std::to_string(ss_tables_.size()) + ".sst.sdb";

  // This bock is important because
  // file_writer closes file in destructor
  {
    auto file_writer = co_await io_manager_.CreateFileWriter(file_path);
    auto builder = co_await disk::SSTableBuilder::Create(file_writer, kPageSize);
    co_await strategy->CompactRecords(merge_iterator, builder);
    co_await std::move(builder).Finish();
    SPDLOG_INFO("SSTableBuilder finished");
  }

  auto file_reader = co_await io_manager_.CreateFileReader(file_path);
  auto ss_table = co_await SSTable::Create(std::move(file_reader));
  ss_tables_.clear();
  ss_tables_.push_back(std::move(ss_table));

  SPDLOG_INFO("Compaction finished");
}

}
