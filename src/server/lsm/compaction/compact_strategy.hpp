#pragma once

#include <lsm/iterators/iterator.hpp>
#include <lsm/disk/ss_table_builder.hpp>

namespace structuredb::server::lsm {

/// @brief describes strategy of compaction
class CompactionStrategy {
public:
  using Ptr = std::shared_ptr<CompactionStrategy>;

  /// @brief action that should be performed on record while compaction
  ///
  /// @p records already persisted records across multiple ss_tables, all records sorted by its key
  /// @p ss_table_builder builder for newly created ss_table
  ///
  /// Use this function if it is required to filter some record
  virtual Awaitable<void> CompactRecords(Iterator::Ptr records, disk::SSTableBuilder& ss_table_builder) = 0;

  virtual ~CompactionStrategy() = default;
};

}
