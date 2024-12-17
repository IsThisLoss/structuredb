#pragma once

#include <io/manager.hpp>
#include <lsm/lsm.hpp>
#include <wal/writer.hpp>

namespace structuredb::server::table {

/// @brief table storage over LSM tree
///
/// This wrapper of Lsm adds two inportant features
/// - logging into the wal
/// - ability to recover from wal
class LsmStorage {
public:
  using Ptr = std::shared_ptr<LsmStorage>;
  using Id = std::string;

  explicit LsmStorage(io::Manager& io_manager, std::string base_dir, std::string id);

  Awaitable<void> Init();

  /// @brief attach LsmStorage to @p wal_writer
  void StartLogInto(wal::Writer::Ptr wal_writer);

  /// @brief restore logged values from wal
  Awaitable<void> RecoverFromLog(const lsm::Sequence seq_no, const std::string& key, const std::string& value);

  /// @brief insert or updates @p value by @p key
  Awaitable<void> Upsert(const std::string& key, const std::string& value);

  /// @brief returns lates value by @p key
  Awaitable<std::optional<std::string>> Get(const std::string& key);

  /// @brief returns iterator over all value versions of @p key
  Awaitable<lsm::Iterator::Ptr> Scan(const std::string& key);

  /// @brief returns iterator over key range between @p lower_bound @p upper_bound
  ///
  /// if @p lower_bound is nullopt, iterator starts from the begining
  /// if @p upper_bound is nullopt, iterator stops only after reading all values
  /// 
  /// Both borders is inclusive, for example, lsm contains keys {a, b, c, d, e}
  /// and we have lower_bound = b, upper_bound = d
  /// when iterator wil return values {b, c, d}
  Awaitable<lsm::Iterator::Ptr> Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound);

  Awaitable<void> Compact(lsm::CompactionStrategy::Ptr strategy);
private:
  const Id id_;
  lsm::Lsm lsm_;
  wal::Writer::Ptr wal_writer_{nullptr};
  lsm::Sequence seq_no_{0};
};

}
