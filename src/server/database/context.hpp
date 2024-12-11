#pragma once

#include <io/manager.hpp>
#include <lsm/lsm.hpp>
#include <wal/writer.hpp>
#include <transaction/storage.hpp>

namespace structuredb::server::database {

/// @brief database common structures
struct Context {
  io::Manager& io_manager;
  std::string base_dir;
  wal::Writer::Ptr wal_writer;
  std::unordered_map<std::string, table::LsmStorage::Ptr> storages;
  transaction::Storage::Ptr tx_storage;
};

}
