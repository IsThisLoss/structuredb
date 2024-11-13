#include "ss_table.hpp"

namespace structuredb::server::lsm {

SSTable::SSTable(disk::File file)
  : file_{std::move(file)}
{}

std::optional<std::string> SSTable::Get(const std::string& key) const {
  return file_.Find(key);
}

}
