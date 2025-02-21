#pragma once

#include <io/manager.hpp>
#include <table/storage/storage.hpp>
#include <transaction/storage.hpp>
#include <wal/writer.hpp>

namespace structuredb::server::database {

/// @brief database common structures
struct Context {
  io::Manager& io_manager;
  std::string base_dir;
  wal::Writer::Ptr wal_writer;
  std::unordered_map<std::string, table::storage::Storage::Ptr> storages;
  transaction::Storage::Ptr tx_storage;
};

}
