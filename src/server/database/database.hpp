#pragma once

#include <io/manager.hpp>
#include <lsm/lsm.hpp>
#include <wal/writer.hpp>
#include <table/table.hpp>

namespace structuredb::server::database {

class Database {
public:
  explicit Database(io::Manager& io_manager, const std::string& base_dir);

  Awaitable<void> Init();

  table::Table::Ptr GetTable();

  void SetTx(int64_t tx);

  int64_t GetNextTx();
private:
  io::Manager& io_manager_;
  const std::string base_dir_;

  wal::Writer::Ptr wal_writer_;

  table::Table::Ptr table_;

  int64_t tx_{1};
};

}
