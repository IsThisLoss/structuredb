#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <io/file_reader.hpp>
#include <cache/lru_cache.hpp>

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
  static Awaitable<SSTable> Create(io::FileReader::Ptr file_reader, size_t page_cache_capacity);

  /// @returns iterator to scan given @p range
  Awaitable<Iterator::Ptr> Scan(const ScanRange& range);

  /// @returns max seq_no from this file
  ///
  /// it is required to keep track what wal records was persisted or not
  Sequence GetMaxSeqNo() const;

  const std::string& GetFilePath() const;
private:
  explicit SSTable(io::FileReader::Ptr file_reader, size_t page_cache_capacity);

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

  cache::LruCache<size_t, disk::Page::Ptr> page_cache_;

  /// @brief returns page by its number
  Awaitable<disk::Page::Ptr> GetPage(int64_t page_num);

  /// @brief returns number of the first page that contains @p key
  Awaitable<int64_t> LowerBound(const std::string& key);

  friend class SSTableIterator;
};

}
