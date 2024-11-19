#include "database.hpp"

#include <iostream>

#include <wal/recovery.hpp>

namespace structuredb::server::database {

Database::Database(io::Manager& io_manager, const std::string& base_dir)
  : io_manager_{io_manager}
  , base_dir_{base_dir}
{
  io_manager_.CoSpawn([this]() -> Awaitable<void> {
      co_await Init();
      std::cerr << "DB initialized!\n";
  });
}

Awaitable<void> Database::Init() {
  // init sys tables
  tx_table_ = std::make_shared<table::LoggedTable>(io_manager_, base_dir_ + "/sys_transactions", "sys_transactions");
  tx_storage_ = std::make_shared<transaction::Storage>(tx_table_);

  // init tables
  table_ = std::make_shared<table::Table>(std::make_shared<table::LoggedTable>(io_manager_, base_dir_ + "/table", "table"), tx_storage_);

  // recovery
  const auto wal_path = base_dir_ + "/wal.sdb";
  const auto control_path = base_dir_ + "/control.sdb";
  co_await wal::Recover(io_manager_, wal_path, control_path, *this);

  // start wal
  wal_writer_ = co_await wal::Open(io_manager_, wal_path, control_path);
  tx_table_->StartLogInto(wal_writer_);
  table_->StartLogInto(wal_writer_);
}

transaction::Storage::Ptr Database::GetTransactionStorage() {
  return tx_storage_;
}

table::Table::Ptr Database::GetTable(const std::string& table_name) {
  return table_;
}

table::LoggedTable::Ptr Database::GetTxTable(const std::string& table_name) {
  return tx_table_;
}

}
