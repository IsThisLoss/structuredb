#pragma once

#include <cstddef>
#include <cstdint>

namespace structuredb::server::lsm {

/// @brief tunable LSM tree parameters
struct Options {
  /// @brief max records in the active mem table before it is frozen
  size_t max_records_in_mem_table{10000};

  /// @brief max read-only mem tables kept in memory before the oldest is
  /// flushed to disk
  size_t max_ro_mem_tables{1};

  /// @brief size of a page in bytes used when writing sstable files
  ///
  /// a single record (key + value + per-record overhead) must fit into one
  /// page, so this also caps the maximum record size
  int64_t page_size{4096};
};

}
