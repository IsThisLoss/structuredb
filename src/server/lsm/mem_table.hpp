#pragma once

#include <map>

#include "ss_table.hpp"

namespace structuredb::server::lsm {

class MemTable {
public:
  void Put(const std::string& key, const std::string& value);

  std::optional<std::string> Get(const std::string& key) const;

  size_t Size() const;

  SSTable Flush(const std::string& file_path) const;
private:
  std::map<std::string, std::string> impl_;
};

}
