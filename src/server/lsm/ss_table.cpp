#include "ss_table.hpp"

namespace structuredb::server::lsm {

SSTable::SSTable(const std::string& file_path)
  : file_{disk::File::Load(file_path)}
{}

std::optional<std::string> SSTable::Get(const std::string& key) const {
  return file_.Find(key);
}

}
