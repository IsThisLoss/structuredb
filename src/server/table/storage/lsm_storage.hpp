#pragma once

#include <io/manager.hpp>
#include <lsm/lsm.hpp>
#include <wal/writer.hpp>

#include "storage.hpp"

namespace structuredb::server::table::storage {

/// @brief table storage over LSM tree
///
/// This wrapper of Lsm adds two important features
/// - logging into the wal
/// - ability to recover from wal
class LsmStorage : public Storage {
public:
  using Ptr = std::shared_ptr<LsmStorage>;

  explicit LsmStorage(io::Manager& io_manager, std::string base_dir, Storage::Id id);

  Awaitable<void> Init();

  void StartLogInto(wal::Writer::Ptr wal_writer) override;

  Awaitable<void> RecoverFromLog(const lsm::Sequence seq_no, const std::string& key, const std::string& value) override;

  Awaitable<void> Upsert(const Row& row) override;

  Awaitable<Iterator::Ptr> Scan(const std::string& key) override;

  Awaitable<Iterator::Ptr> Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound) override;

  Awaitable<void> Compact(CompactionStrategy::Ptr strategy) override;
private:
  const Id id_;
  lsm::Lsm lsm_;
  wal::Writer::Ptr wal_writer_{nullptr};
  lsm::Sequence seq_no_{0};
};

}
