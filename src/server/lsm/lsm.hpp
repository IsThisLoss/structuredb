#pragma once

#include <io/manager.hpp>

#include "mem_table.hpp"
#include "ss_table.hpp"

namespace structuredb::server::lsm {

class Lsm {
public:
  explicit Lsm(io::Manager& io_manager);

  Awaitable<void> Put(const std::string& key, const std::string& value);

  Awaitable<std::optional<std::string>> Get(const std::string& key);
private:
  constexpr static const size_t kMaxTableSize{1};

  constexpr static const size_t kMaxRoMemTables{1};

  io::Manager& io_manager_;

  MemTable mem_table_;
  std::vector<MemTable> ro_mem_tables_;
  std::vector<SSTable> ss_tables_;
};

}
