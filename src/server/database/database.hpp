#pragma once

#include <io/manager.hpp>
#include <lsm/lsm.hpp>
#include <wal/writer.hpp>
#include <table/table.hpp>
#include <transaction/storage.hpp>

namespace structuredb::server::database {

class Database {
public:
  explicit Database(io::Manager& io_manager, const std::string& base_dir);

  Awaitable<void> Init();

  Awaitable<void> CreateTable(const transaction::TransactionId& tx, const std::string& name);

  Awaitable<void> DropTable(const transaction::TransactionId& tx, const std::string& name);

  table::Table::Ptr GetTableForRecover(const std::string& table_name);

  Awaitable<table::Table::Ptr> GetTable(const transaction::TransactionId& tx, const std::string& table_name);

  table::LoggedTable::Ptr GetTxTable();

  transaction::Storage::Ptr GetTransactionStorage();
private:
  io::Manager& io_manager_;
  const std::string base_dir_;

  wal::Writer::Ptr wal_writer_;

  table::LoggedTable::Ptr tx_table_;
  table::Table::Ptr sys_tables_;

  std::unordered_map<std::string, table::Table::Ptr> tables_;

  transaction::Storage::Ptr tx_storage_;
};

}
