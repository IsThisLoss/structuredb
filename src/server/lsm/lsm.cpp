#include "lsm.hpp"

#include <iostream>

namespace structuredb::server::lsm {

void Lsm::Put(const std::string& key, const std::string& value) {
  mem_table_.Put(key, value);
  if (mem_table_.Size() > kMaxTableSize) {
    std::cerr << "Mem table reached max size, freeze it\n";
    ro_mem_tables_.push_back(std::move(mem_table_));
    mem_table_ = MemTable{};
  }
}

std::optional<std::string> Lsm::Get(const std::string& key) {
  auto value = mem_table_.Get(key);
  if (value.has_value()) {
    return value;
  }

  std::cerr << "Did not find key " << key << " in active mem table, will search in frozen mem tables\n";
  for (auto it = ro_mem_tables_.rbegin(); it != ro_mem_tables_.rend(); ++it) {
    value = it->Get(key);
    if (value.has_value()) {
      return value;
    }
  }

  return std::nullopt;
}

}
