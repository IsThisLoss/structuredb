#include "lsm.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <spdlog/spdlog.h>

#include "iterators/lsm_range_iterator.hpp"
#include "iterators/lsm_key_iterator.hpp"
#include "iterators/merge_iterator.hpp"

#include <boost/asio/this_coro.hpp>

namespace structuredb::server::lsm {

namespace {

int64_t GetSSTableNo(const std::string& file_path) {
  auto pos = file_path.find_last_of('/');
  try {
    return std::stoll(file_path.substr(pos + 1));
  } catch (const std::exception& ex) {
    return -1;
  }
}

}

Lsm::Lsm(io::Manager& io_manager, std::string base_dir)
  : io_manager_{io_manager}
  , base_dir_{std::move(base_dir)}
  , shared_mutex_{io_manager_.Context()}
{}

Awaitable<void> Lsm::Init() {
  Sequence max_persistent_seq_no{};
  auto names = co_await io_manager_.ListDirectory(base_dir_);
  std::ranges::sort(names);
  for (const auto& name : names) {
    auto file_reader = co_await io_manager_.CreateFileReader(base_dir_ + "/" + name);
    auto ss_table = co_await SSTable::Create(std::move(file_reader));
    max_persistent_seq_no_ = std::max(max_persistent_seq_no_, ss_table.GetMaxSeqNo());
    ss_tables_.push_back(std::move(ss_table));
  }
  next_seq_no_ = max_persistent_seq_no_ + 1;
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

Sequence Lsm::GetMaxPersistentSeqNo() const {
  return max_persistent_seq_no_;
}

Awaitable<void> Lsm::DoPut(const Sequence seq_no, const std::string& key, const std::string& value) {
  co_await shared_mutex_.LockExclusive();

  mem_table_.Put(Record{.key = key, .seq_no = seq_no, .value = value});

  if (mem_table_.Size() > kMaxRecordsInMemTable) {
    SPDLOG_INFO("Mem table reached max size, freeze it");
    ro_mem_tables_.push_back(std::move(mem_table_));
    mem_table_ = MemTable{};
  }

  if (ro_mem_tables_.size() > kMaxRoMemTables) {
    SPDLOG_INFO("Ro Mem tables reached max size, flush it");
    const auto file_path = std::format("{}/{:04d}.sst.sdb", base_dir_, ss_tables_.size());
    auto ss_table = co_await ro_mem_tables_.front().Flush(io_manager_, file_path);
    max_persistent_seq_no_ = std::max(max_persistent_seq_no_, ss_table.GetMaxSeqNo());
    ss_tables_.push_back(std::move(ss_table));
    ro_mem_tables_.erase(ro_mem_tables_.begin());
  }

  co_await shared_mutex_.UnlockExclusive();
}

Awaitable<std::optional<std::string>> Lsm::Get(const std::string& key) {
  co_await shared_mutex_.LockShared();

  std::optional<std::string> result;
  const auto iterator = co_await Scan(key);
  while (iterator->HasMore()) {
    auto record = co_await iterator->Next();
    result = record.value;
    break;
  }

  co_await shared_mutex_.UnlockShared();
  co_return result;
}

Awaitable<Iterator::Ptr> Lsm::Scan(const std::string& key) {
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

  std::vector<std::string> files_to_delete;
  files_to_delete.reserve(ss_tables_.size());
  std::vector<Iterator::Ptr> iterators;
  iterators.reserve(ss_tables_.size());
  for (auto& ss_table : ss_tables_) {
    iterators.push_back(co_await ss_table.Scan(ScanRange::FullScan()));
    files_to_delete.push_back(ss_table.GetFilePath());
  }
  auto merge_iterator = std::make_shared<MergeIterator>(co_await MergeIterator::Create(std::move(iterators)));

  int64_t new_sst_no = 0;
  if (!ss_tables_.empty()) {
    new_sst_no = GetSSTableNo(ss_tables_.back().GetFilePath()) + 1;
  }

  const auto file_path = std::format("{}/{:04d}.sst.sdb", base_dir_, new_sst_no);

  // This bock is important because
  // file_writer closes file in destructor
  {
    SPDLOG_INFO("SSTableBuilder started");
    auto file_writer = co_await io_manager_.CreateFileWriter(file_path);
    auto builder = co_await disk::SSTableBuilder::Create(file_writer, kPageSize);
    co_await strategy->CompactRecords(merge_iterator, builder);
    co_await std::move(builder).Finish();
    SPDLOG_INFO("SSTableBuilder finished");
  }

  auto file_reader = co_await io_manager_.CreateFileReader(file_path);
  auto ss_table = co_await SSTable::Create(std::move(file_reader));

  co_await shared_mutex_.LockExclusive();
  ss_tables_.clear();
  ss_tables_.push_back(std::move(ss_table));
  co_await shared_mutex_.UnlockExclusive();

  for (const auto& path : files_to_delete) {
    co_await io_manager_.Remove(path);
  }

  SPDLOG_INFO("Compaction finished, new ss_tables count: {}", ss_tables_.size());
}

int Lsm::CountSSTables() const {
  return static_cast<int>(ss_tables_.size());
}

}
