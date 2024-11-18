#pragma once

#include <list>

#include <io/manager.hpp>

#include "mem_table.hpp"
#include "ss_table.hpp"

namespace structuredb::server::lsm {

/// @brief Log Structure Merge Tree
class Lsm {
public:
  explicit Lsm(io::Manager& io_manager, const std::string& base_dir);

  Sequence GetMaxPersistentSeqNo() const;

  /// @brief add or update data
  ///
  /// returns mem table if flush on disk was performed
  Awaitable<std::optional<MemTable>> Put(const std::string& key, const Sequence seq_no, const std::string& value);

  /// @brief retrives value by key
  ///
  /// @p consume will be called for each value assossiated with @p key
  Awaitable<void> Get(const std::string& key, const RecordConsumer& consume);
private:
  constexpr static const size_t kMaxTableSize{50};

  constexpr static const size_t kMaxRoMemTables{1};

  io::Manager& io_manager_;
  const std::string base_dir_{};

  MemTable mem_table_;
  std::list<MemTable> ro_mem_tables_{};
  std::vector<SSTable> ss_tables_{};

  Sequence max_persistent_seq_no_{0};
};

}
