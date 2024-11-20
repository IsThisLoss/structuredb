#pragma once

#include <io/manager.hpp>
#include <table/table.hpp>

namespace structuredb::server::database {

class Tables {
public:
  explicit Tables(io::Manager& io_manager, std::string base_dir, table::Table::Ptr table);

  Awaitable<void> Create(const transaction::TransactionId& tx, const std::string& name);

  Awaitable<void> Drop(const transaction::TransactionId& tx, const std::string& name);

private:
  io::Manager& io_manager_;
  const std::string base_dir_;
  table::Table::Ptr table_;
};

}
