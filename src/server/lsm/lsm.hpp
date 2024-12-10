#pragma once

#include <list>

#include <io/manager.hpp>

#include <wal/writer.hpp>

#include "iterators/iterator.hpp"

#include "mem_table.hpp"
#include "ss_table.hpp"

namespace structuredb::server::lsm {

class LsmRangeIterator;

/// @brief Log Structure Merge Tree
class Lsm {
public:
  explicit Lsm(io::Manager& io_manager, const std::string& base_dir);

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

  /// @brief retrives lates value by key
  Awaitable<std::optional<std::string>> Get(const std::string& key);

  /// @brief retrives all value's versions by key
  ///
  /// @p consume will be called for each value version assossiated with @p key
  Awaitable<void> Scan(const std::string& key, const RecordConsumer& consume);

  /// @brief scan lsm tree by range of keys
  Awaitable<Iterator::Ptr> Scan(const ScanRange& range);
private:
  constexpr static const size_t kMaxRecordsInMemTable{50};

  constexpr static const size_t kMaxRoMemTables{1};

  io::Manager& io_manager_;
  const std::string base_dir_{};

  MemTable mem_table_;
  std::list<MemTable> ro_mem_tables_{};
  std::vector<SSTable> ss_tables_{};

  Sequence next_seq_no_{0};

  Awaitable<void> DoPut(const Sequence seq_no, const std::string& key, const std::string& value);

  friend class LsmRangeIterator;
};

}
