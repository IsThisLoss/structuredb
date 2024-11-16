#include "lsm.hpp"

#include <iostream>

namespace structuredb::server::lsm {

Lsm::Lsm(io::Manager& io_manager, const std::string& base_dir)
  : io_manager_{io_manager}
  , base_dir_{base_dir}
{}

Awaitable<void> Lsm::Put(const std::string& key, const std::string& value) {
  mem_table_.Put(key, value);

  if (mem_table_.Size() > kMaxTableSize) {
    std::cerr << "Mem table reached max size, freeze it\n";
    ro_mem_tables_.push_back(std::move(mem_table_));
    mem_table_ = MemTable{};
  }

  if (ro_mem_tables_.size() > kMaxRoMemTables) {
    std::cerr << "Ro Mem tables reached max size, flush it\n";
    const auto file_path = base_dir_ + std::to_string(ss_tables_.size()) + ".sst.sdb";
    auto ss_table = co_await ro_mem_tables_.front().Flush(io_manager_, file_path);
    ss_tables_.push_back(std::move(ss_table));
    // FIXME use circular buffer
    ro_mem_tables_.erase(ro_mem_tables_.begin());
  }
}

Awaitable<void> Lsm::Get(const std::string& key, const RecordConsumer& consume) {
  mem_table_.Get(key, consume);

  // TODO use Bloom Filter
  for (auto it = ro_mem_tables_.rbegin(); it != ro_mem_tables_.rend(); ++it) {
    it->Get(key, consume);
  }

  for (auto it = ss_tables_.rbegin(); it != ss_tables_.rend(); ++it) {
    co_await it->Get(key, consume);
  }
}

}
