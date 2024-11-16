#include "database.hpp"

#include <wal/recovery.hpp>

namespace structuredb::server::database {

/*
Awaitable<Database> Database::Create(io::Manager& io_manager, const std::string& base_dir) {
  Database database{io_manager, base_dir};
  co_await database.Init();
  co_return database;
}
*/

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
  table_ = std::make_shared<table::Table>(io_manager_, base_dir_ + "/table");

  // recovery
  const auto wal_path = base_dir_ + "/wal.sdb";
  co_await wal::Recover(io_manager_, wal_path, *this);

  // start wal
  wal_writer_ = co_await wal::Open(io_manager_, wal_path);
  table_->StartWal(wal_writer_);
}

table::Table::Ptr Database::GetTable() {
  return table_;
}

int64_t Database::GetNextTx() {
  return tx_++;
}

}
