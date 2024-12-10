#pragma once

#include <set>

#include <io/manager.hpp>

#include "iterators/iterator.hpp"
#include "ss_table.hpp"

namespace structuredb::server::lsm {

class MemTableIterator;

class MemTable {
public:
  void Put(Record&& record);

  bool Scan(const std::string& key, const RecordConsumer& consume) const;

  void ScanValues(const RecordConsumer& consume) const;

  size_t Size() const;

  Awaitable<SSTable> Flush(io::Manager& io_manager, const std::string& file_path) const;

  Iterator::Ptr Scan(const ScanRange& range) const;

private:
  std::multiset<Record> impl_;

  friend class MemTableIterator;
};

}
