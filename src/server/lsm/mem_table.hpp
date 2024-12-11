#pragma once

#include <set>

#include <io/manager.hpp>

#include "iterators/iterator.hpp"
#include "ss_table.hpp"

namespace structuredb::server::lsm {

/// @brief memory table
///
/// keeps key, value pairs sorted by key in memory
class MemTable {
public:
  /// @brief adds new record
  void Put(Record&& record);

  /// @brief returns size of MemTable
  size_t Size() const;

  /// @brief creates SSTable based on current MemTable
  ///
  /// (Persists MemTable data on disk)
  Awaitable<SSTable> Flush(io::Manager& io_manager, const std::string& file_path) const;

  /// @returns iterator to scan given @p range
  Iterator::Ptr Scan(const ScanRange& range) const;

private:
  std::multiset<Record> impl_;

  friend class MemTableIterator;
};

}
