#pragma once

#include "mem_table.hpp"
#include "ss_table.hpp"

namespace structuredb::server::lsm {

class Lsm {
public:
  explicit Lsm(boost::asio::io_context& io_context);

  boost::asio::awaitable<void> Put(const std::string& key, const std::string& value);

  std::optional<std::string> Get(const std::string& key);
private:
  constexpr static const size_t kMaxTableSize{2};

  constexpr static const size_t kMaxRoMemTables{1};

  boost::asio::io_context& io_context_;

  MemTable mem_table_;
  std::vector<MemTable> ro_mem_tables_;
  std::vector<SSTable> ss_tables_;
};

}
