#include "lsm.hpp"

#include <cstdint>
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

Lsm::Lsm(io::Manager& io_manager, std::string base_dir, Options options)
  : io_manager_{io_manager}
  , base_dir_{std::move(base_dir)}
  , options_{options}
  , shared_mutex_{io_manager_.Context()}
  , maintenance_mutex_{io_manager_.Context()}
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

  if (mem_table_.Size() > options_.max_records_in_mem_table) {
    SPDLOG_INFO("Mem table reached max size, freeze it");
    ro_mem_tables_.push_back(std::move(mem_table_));
    mem_table_ = MemTable{};

    // The actual flush to disk is done by a background job (see Flush()), so
    // the write path stays off the disk. If the flusher falls behind, frozen
    // mem tables pile up in memory - warn so it is observable.
    if (ro_mem_tables_.size() > options_.max_ro_mem_tables) {
      SPDLOG_WARN(
          "Flush is falling behind: {} frozen mem tables in memory (limit {})",
          ro_mem_tables_.size(), options_.max_ro_mem_tables);
    }
  }

  co_await shared_mutex_.UnlockExclusive();
}

Awaitable<void> Lsm::Flush() {
  // serialize against Compact (both mutate ss_tables_)
  co_await maintenance_mutex_.LockExclusive();

  // How many frozen mem tables to drain in this run. New ones frozen while we
  // are flushing are left for the next run, so a constant write load cannot
  // starve this loop.
  co_await shared_mutex_.LockShared();
  auto pending = ro_mem_tables_.size();
  co_await shared_mutex_.UnlockShared();

  while (pending > 0) {
    // The front frozen mem table is immutable: the write path only appends new
    // frozen tables (std::deque never invalidates references on push_back) and
    // this single, sequentially-run job is the only place that pops them. So
    // the slow disk write below can run without holding any lock, keeping the
    // write path (which needs the exclusive lock) unblocked.
    const auto file_path = std::format("{}/{:04d}.sst.sdb", base_dir_, ss_tables_.size());
    auto ss_table = co_await ro_mem_tables_.front().Flush(io_manager_, file_path, options_.page_size);

    // Publish the new ss table and drop the now-persisted mem table atomically.
    // The mem table stays readable (in ro_mem_tables_) for the whole flush, so
    // concurrent reads never lose its keys.
    co_await shared_mutex_.LockExclusive();
    max_persistent_seq_no_ = std::max(max_persistent_seq_no_, ss_table.GetMaxSeqNo());
    ss_tables_.push_back(std::move(ss_table));
    ro_mem_tables_.pop_front();
    co_await shared_mutex_.UnlockExclusive();

    --pending;
  }

  co_await maintenance_mutex_.UnlockExclusive();
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

  // serialize against background Flush so ss_tables_ is not mutated underneath
  // the scan/merge below
  co_await maintenance_mutex_.LockExclusive();

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
    auto builder = co_await disk::SSTableBuilder::Create(file_writer, options_.page_size);
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

  co_await maintenance_mutex_.UnlockExclusive();

  SPDLOG_INFO("Compaction finished, new ss_tables count: {}", ss_tables_.size());
}

int Lsm::CountSSTables() const {
  return static_cast<int>(ss_tables_.size());
}

}
