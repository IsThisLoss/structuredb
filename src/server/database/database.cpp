#include "database.hpp"

#include <wal/recovery.hpp>

namespace structuredb::server::database {

Database::Database(io::Manager& io_manager, const std::string& base_dir)
  : io_manager_{io_manager}
  , base_dir_{base_dir}
{
  io_manager_.CoSpawn([this]() -> Awaitable<void> {
      co_await Init();
  });
}

Awaitable<void> Database::Init() {
  // init tables
  table_ = std::make_shared<table::Table>(io_manager_, base_dir_ + "/table", *this);

  // recovery
  const auto wal_path = base_dir_ + "/wal.sdb";
  const auto control_path = base_dir_ + "/control.sdb";
  co_await wal::Recover(io_manager_, wal_path, control_path, *this);

  // start wal
  wal_writer_ = co_await wal::Open(io_manager_, wal_path, control_path);
  table_->StartLogInto(wal_writer_);
}

transaction::Storage& Database::GetTransactionStorage() {
  return tx_storage_;
}

table::Table::Ptr Database::GetTable() {
  return table_;
}

}
