#pragma once

#include "mem_table.hpp"
#include "ss_table.hpp"

namespace structuredb::server::lsm {

class Lsm {
public:
  void Put(const std::string& key, const std::string& value);

  std::optional<std::string> Get(const std::string& key);
private:
  constexpr static const size_t kMaxTableSize{10};
  MemTable mem_table_;
  std::vector<MemTable> ro_mem_tables_;
  std::vector<SSTable> ss_tables_;
};

}
