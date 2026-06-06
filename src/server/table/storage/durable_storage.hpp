#pragma once

#include <memory>
#include <string>

#include <types.hpp>
#include <io/manager.hpp>
#include <wal/writer.hpp>

namespace structuredb::server::table::storage {

/// @brief WAL-durability capability for persistent storage engines
///
/// Kept separate from StorageEngine so engines without write-ahead
/// durability (e.g. a future in-memory engine) needn't implement
/// LSM-shaped recovery. Only persistent engines (LsmEngine) implement it,
/// and the recovery path (wal::LsmStorageUpsertEvent) talks to engines
/// through this interface.
class DurableStorage {
public:
  using Ptr = std::shared_ptr<DurableStorage>;

  /// @brief attach storage to @p wal_writer
  virtual void StartLogInto(wal::Writer::Ptr wal_writer) = 0;

  /// @brief restore a logged value from the wal
  virtual Awaitable<void> RecoverFromLog(const types::Sequence seq_no, const std::string& key, const std::string& value) = 0;

  /// @brief whether the record at @p seq_no is already persisted on disk
  virtual Awaitable<bool> IsPersistent(const types::Sequence seq_no) = 0;

  virtual ~DurableStorage() = default;
};

}
