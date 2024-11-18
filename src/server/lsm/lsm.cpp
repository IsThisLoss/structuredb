#include "lsm.hpp"

#include <iostream>
#include <filesystem>

namespace structuredb::server::lsm {

Lsm::Lsm(io::Manager& io_manager, const std::string& base_dir)
  : io_manager_{io_manager}
  , base_dir_{base_dir}
{
  io_manager_.CoSpawn([this]() -> Awaitable<void> {
      for (const auto & dir_entry : std::filesystem::directory_iterator{base_dir_}) {
        auto file_reader = co_await io_manager_.CreateFileReader(dir_entry.path());
        auto ss_table = co_await SSTable::Create(std::move(file_reader));
        max_persistent_seq_no_ = std::max(max_persistent_seq_no_, ss_table.GetMaxSeqNo());
        ss_tables_.push_back(std::move(ss_table));
      }
      std::cerr << "SSTables ready!\n";
      co_return;
  });
}

Sequence Lsm::GetMaxPersistentSeqNo() const {
  return max_persistent_seq_no_;
}

Awaitable<std::optional<MemTable>> Lsm::Put(const std::string& key, const Sequence seq_no, const std::string& value) {
  std::optional<MemTable> result;

  mem_table_.Put(Record{key, seq_no, value});

  if (mem_table_.Size() > kMaxTableSize) {
    std::cerr << "Mem table reached max size, freeze it\n";
    ro_mem_tables_.push_back(std::move(mem_table_));
    mem_table_ = MemTable{};
  }

  if (ro_mem_tables_.size() > kMaxRoMemTables) {
    std::cerr << "Ro Mem tables reached max size, flush it\n";
    const auto file_path = base_dir_ + "/" + std::to_string(ss_tables_.size()) + ".sst.sdb";
    auto ss_table = co_await ro_mem_tables_.front().Flush(io_manager_, file_path);
    result = std::move(ro_mem_tables_.front());
    ss_tables_.push_back(std::move(ss_table));
    // FIXME use circular buffer
    ro_mem_tables_.erase(ro_mem_tables_.begin());
  }

  co_return result;
}

Awaitable<void> Lsm::Get(const std::string& key, const RecordConsumer& consume) {
  if (mem_table_.Get(key, consume)) {
    co_return;
  }

  // TODO use Bloom Filter
  for (auto it = ro_mem_tables_.rbegin(); it != ro_mem_tables_.rend(); ++it) {
    if (it->Get(key, consume)) {
      co_return;
    }
  }

  for (auto it = ss_tables_.rbegin(); it != ss_tables_.rend(); ++it) {
    if (co_await it->Get(key, consume)) {
      co_return;
    }
  }
}

}
