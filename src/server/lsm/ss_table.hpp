#pragma once

#include <string>

#include <io/file_reader.hpp>

#include "disk/page.hpp"
#include "disk/ss_table_header.hpp"
#include "iterators/iterator.hpp"

namespace structuredb::server::lsm {

/// @brief sorted string table
///
/// Each SSTable is a file on disk
/// File contains sorted list of keys and values
/// Also file is divided on pages
class SSTable {
public:
  static Awaitable<SSTable> Create(io::FileReader::Ptr file_reader);

  /// @returns iterator to scan given @p range
  Awaitable<Iterator::Ptr> Scan(const ScanRange& range);

  /// @returns max seq_no from this file
  ///
  /// it is required to keep track what wal records was persisted or not
  Sequence GetMaxSeqNo() const;
private:
  explicit SSTable(io::FileReader::Ptr file_reader);

  Awaitable<void> Init();

  /***** Structure of file ******/
  /* SSTableHeader:
   *   page_size: int64
   *   page_count: int64
   * Pages:
   *   Page:
   *     PageHeader:
   *       size:   int64
   *    keys:   string[]
   *    values: string[]
   */
  io::FileReader::Ptr file_reader_{};
  disk::SSTableHeader header_{};
  int64_t header_size_{};
  disk::Page page_{};

  /// TODO use lru cache
  std::unordered_map<size_t, disk::Page> page_cache_;

  /// @brief returns page by its number
  Awaitable<disk::Page> GetPage(int64_t page_num);

  /// @brief returns number of the first page that contains @p key
  Awaitable<int64_t> LowerBound(const std::string& key);

  friend class SSTableIterator;
};

}
