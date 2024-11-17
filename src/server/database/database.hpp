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

  table::Table::Ptr GetTable();

  transaction::Storage& GetTransactionStorage();
private:
  io::Manager& io_manager_;
  const std::string base_dir_;

  wal::Writer::Ptr wal_writer_;

  table::Table::Ptr table_;

  transaction::Storage tx_storage_;
};

}
