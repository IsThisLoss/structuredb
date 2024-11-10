#pragma once

#include <map>

namespace structuredb::server::lsm {

class MemTable {
public:
  void Put(const std::string& key, const std::string& value);

  std::optional<std::string> Get(const std::string& key) const;

  size_t Size() const;
private:
  std::map<std::string, std::string> impl_;
};

}
