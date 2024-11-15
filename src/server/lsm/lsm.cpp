#include "lsm.hpp"

#include <iostream>

namespace structuredb::server::lsm {

namespace {

// FIXME create path in runtime
const std::string kFilePath = "/tmp/structuredb.sstable";

}

Lsm::Lsm(io::Manager& io_manager)
  : io_manager_{io_manager}
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
    const auto file_path = "/tmp/sstable_" + std::to_string(ss_tables_.size()) + ".structuredb";
    auto ss_table = co_await ro_mem_tables_.front().Flush(io_manager_, file_path);
    ss_tables_.push_back(std::move(ss_table));
    // FIXME use circular buffer
    ro_mem_tables_.erase(ro_mem_tables_.begin());
  }
}

Awaitable<std::optional<std::string>> Lsm::Get(const std::string& key) {
  auto value = mem_table_.Get(key);
  if (value.has_value()) {
    co_return value;
  }

  std::cerr << "Did not find key " << key << " in active mem table, will search in frozen mem tables\n";

  // TODO use Bloom Filter
  for (auto it = ro_mem_tables_.rbegin(); it != ro_mem_tables_.rend(); ++it) {
    value = it->Get(key);
    if (value.has_value()) {
      co_return value;
    }
  }

  std::cerr << "Did not find key " << key << " in ro mem table, will search in ss tables\n";

  for (auto it = ss_tables_.rbegin(); it != ss_tables_.rend(); ++it) {
    value = co_await it->Get(key);
    if (value.has_value()) {
      co_return value;
    }
  }

  co_return std::nullopt;
}

}
