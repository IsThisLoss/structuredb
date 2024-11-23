#include "lsm.hpp"

#include <spdlog/spdlog.h>

namespace structuredb::server::lsm {

Lsm::Lsm(io::Manager& io_manager, const std::string& base_dir)
  : io_manager_{io_manager}
  , base_dir_{base_dir}
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
  if (seq_no != next_seq_no_) {
    co_return false;
  }
  co_await DoPut(seq_no, key, value);
  next_seq_no_++;
  co_return true;
}

Awaitable<void> Lsm::DoPut(const Sequence seq_no, const std::string& key, const std::string& value) {
  mem_table_.Put(Record{key, seq_no, value});

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
  co_await Scan(key, [&](const auto& value) {
      result = value;
      return true;
  });
  co_return result;
}

Awaitable<void> Lsm::Scan(const std::string& key, const RecordConsumer& consume) {
  spdlog::debug("LSM scan key = {}", key);
  if (mem_table_.Scan(key, consume)) {
    co_return;
  }

  spdlog::debug("Did not find in active mem table, will scan ro, key = {}", key);

  // TODO use Bloom Filter
  for (auto it = ro_mem_tables_.rbegin(); it != ro_mem_tables_.rend(); ++it) {
    if (it->Scan(key, consume)) {
      co_return;
    }
  }

  spdlog::debug("Did not find in ro mem tables, will scan ss tables, key = {}", key);

  for (auto it = ss_tables_.rbegin(); it != ss_tables_.rend(); ++it) {
    if (co_await it->Scan(key, consume)) {
      co_return;
    }
  }

  spdlog::debug("Key {} was not found", key);
}

}
