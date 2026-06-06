#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <optional>
#include <deque>

#include <io/manager.hpp>
#include <io/shared_mutex.hpp>
#include <wal/writer.hpp>

#include "iterators/iterator.hpp"
#include "compaction/compact_strategy.hpp"
#include "mem_table.hpp"
#include "options.hpp"
#include "ss_table.hpp"

namespace structuredb::server::lsm {

/// @brief Log Structure Merge Tree
class Lsm {
public:
  explicit Lsm(io::Manager& io_manager, std::string base_dir, Options options = {});

  Awaitable<void> Init();

  /// @brief adds key value to LSM
  ///
  /// returns sequence number of inserted record
  Awaitable<Sequence> Put(const std::string& key, const std::string& value);

  /// @brief inserts with provided sequence
  ///
  /// if sequence is valid next sequence for lsm, inserts record and returns true
  /// otherwise returns false
  Awaitable<bool> Put(const Sequence seq_no, const std::string& key, const std::string& value);

  Sequence GetMaxPersistentSeqNo() const;

  /// @brief retrives lates value by key
  Awaitable<std::optional<std::string>> Get(const std::string& key);

  /// @brief retrives all value's versions by key
  Awaitable<Iterator::Ptr> Scan(const std::string& key);

  /// @brief scan lsm tree by range of keys
  Awaitable<Iterator::Ptr> Scan(const ScanRange& range);

  Awaitable<void> Compact(CompactionStrategy::Ptr strategy);

  /// @brief persists frozen (read-only) mem tables to ss tables
  ///
  /// Intended to be driven by a background job: the write path only freezes
  /// the active mem table, the actual disk write happens here off the
  /// critical path.
  Awaitable<void> Flush();

  int CountSSTables() const;
private:
  io::Manager& io_manager_;
  const std::string base_dir_{};
  const Options options_{};
  // guards mem_table_ / ro_mem_tables_ / ss_tables_ against concurrent
  // readers and writers
  io::SharedMutex shared_mutex_;
  // serializes background maintenance (Flush vs Compact) on this lsm so they
  // never mutate ss_tables_ at the same time
  io::SharedMutex maintenance_mutex_;

  MemTable mem_table_;
  std::deque<MemTable> ro_mem_tables_{};
  std::vector<SSTable> ss_tables_{};

  Sequence max_persistent_seq_no_{0};
  Sequence next_seq_no_{0};

  Awaitable<void> DoPut(const Sequence seq_no, const std::string& key, const std::string& value);

  friend class LsmRangeIterator;
  friend class LsmKeyIterator;
};

}
