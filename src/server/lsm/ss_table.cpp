#include "ss_table.hpp"

namespace structuredb::server::lsm {

SSTable::SSTable(const std::string& file_path) {
  fin.open(file_path, std::ios::binary);
}

SSTable::~SSTable() {
  fin.close();
}

std::optional<std::string> SSTable::Get(const std::string& key) const {
  return std::nullopt;
}

}
