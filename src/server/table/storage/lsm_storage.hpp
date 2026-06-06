#pragma once

#include <memory>
#include <string>
#include <optional>
#include <io/manager.hpp>
#include <lsm/lsm.hpp>
#include <wal/writer.hpp>

#include "storage.hpp"
#include "durable_storage.hpp"

namespace structuredb::server::table::storage {

/// @brief LSM-backed storage engine
///
/// Splits cleanly from lsm::Lsm: Lsm is the bare LSM-tree data structure
/// (key/value, sequences, sstables), while LsmEngine is the engine that
/// adapts it to the Storage interface and owns durability:
/// - logging upserts into the wal
/// - replaying them on recovery
/// - translating between table::Row and lsm::Record
class LsmEngine : public StorageEngine, public DurableStorage {
public:
  using Ptr = std::shared_ptr<LsmEngine>;

  explicit LsmEngine(io::Manager& io_manager, std::string base_dir, StorageEngine::Id id, lsm::Options lsm_options = {});

  Awaitable<void> Init();

  void StartLogInto(wal::Writer::Ptr wal_writer) override;

  Awaitable<void> RecoverFromLog(const types::Sequence seq_no, const std::string& key, const std::string& value) override;

  Awaitable<bool> IsPersistent(const types::Sequence seq_no) override;

  Awaitable<void> Upsert(const Row& row) override;

  Awaitable<Iterator::Ptr> Scan(const std::string& key) override;

  Awaitable<Iterator::Ptr> Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound) override;

  Awaitable<void> Compact(CompactionStrategy::Ptr strategy) override;

  Awaitable<void> Flush() override;

  /// @brief number of on-disk SSTables — LSM-specific statistic, not part of
  /// the engine interface; callers must downcast to query it
  int CountSSTables() const;
private:
  const Id id_;
  lsm::Lsm lsm_;
  wal::Writer::Ptr wal_writer_{nullptr};
};

}
