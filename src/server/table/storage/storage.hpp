#pragma once

#include <memory>

#include <io/manager.hpp>
#include <lsm/lsm.hpp>
#include <table/iterator.hpp>
#include <table/row.hpp>
#include <wal/writer.hpp>

namespace structuredb::server::table::storage {

/// @brief interface of table storage
///
/// Represents storage level of a table
class Storage {
public:
  using Ptr = std::shared_ptr<Storage>;
  using Id = std::string;
  
  /// @brief attach storage to @p wal_writer
  virtual void StartLogInto(wal::Writer::Ptr wal_writer) = 0;

  /// @brief restore logged values from wal
  ///
  /// TODO: change interface to hide seq_no
  virtual Awaitable<void> RecoverFromLog(const lsm::Sequence seq_no, const std::string& key, const std::string& value) = 0;

  /// @brief inserts or updates value by key
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

  virtual Awaitable<void> Compact(lsm::CompactionStrategy::Ptr strategy) = 0;

  virtual ~Storage() = default;
};

}
