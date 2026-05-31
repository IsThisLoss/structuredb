#pragma once

#include <string>
#include <optional>
#include <memory>

#include <io/manager.hpp>
#include <table/iterator.hpp>
#include <table/row.hpp>

#include "compaction_strategy.hpp"

namespace structuredb::server::table::storage {

/// @brief pluggable storage engine interface
///
/// This is the seam for swapping the physical storage engine behind a table.
/// Today the only implementation is LsmEngine, but an in-memory engine could
/// implement the same interface. The engine deals only with opaque key/value
/// rows — transaction visibility and MVCC live one layer up, in the table
/// implementations (see table::TransactionalTable).
class StorageEngine {
public:
  using Ptr = std::shared_ptr<StorageEngine>;
  using Id = std::string;

  /// @brief inserts or updates value by key
  ///
  /// @param row row to upsert
  ///
  /// The engine is transaction-agnostic: MVCC visibility and locking are
  /// handled by the table layer (see TransactionalTable), the row value is
  /// already an opaque blob by the time it reaches the storage engine.
  virtual Awaitable<void> Upsert(const Row& row) = 0;

  /// @brief returns iterator over all value versions of @p key
  virtual Awaitable<Iterator::Ptr> Scan(const std::string& key) = 0;

  /// @brief returns iterator over key range between @p lower_bound @p upper_bound
  ///
  /// if @p lower_bound is nullopt, iterator starts from the begining
  /// if @p upper_bound is nullopt, iterator stops only after reading all values
  /// 
  /// Both borders is inclusive, for example, lsm contains keys {a, b, c, d, e}
  /// and we have lower_bound = b, upper_bound = d
  /// then iterator will return values {b, c, d}
  virtual Awaitable<Iterator::Ptr> Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound) = 0;

  /// @brief runs optimization of storage
  virtual Awaitable<void> Compact(CompactionStrategy::Ptr strategy) = 0;

  virtual ~StorageEngine() = default;
};

}
