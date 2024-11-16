#pragma once

#include <map>

#include <io/manager.hpp>

#include "ss_table.hpp"

namespace structuredb::server::lsm {

class MemTable {
public:
  void Put(const std::string& key, const std::string& value);

  void Get(const std::string& key, const RecordConsumer& consume) const;

  size_t Size() const;

  Awaitable<SSTable> Flush(io::Manager& io_manager, const std::string& file_path) const;
private:
  std::map<std::string, std::string> impl_;
};

}
