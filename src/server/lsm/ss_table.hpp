#pragma once

#include <string>

#include "disk/file.hpp"

namespace structuredb::server::lsm {

class SSTable {
public:
  explicit SSTable(const std::string& file_path);

  std::optional<std::string> Get(const std::string& key) const;
private:
  disk::File file_;
};

}
