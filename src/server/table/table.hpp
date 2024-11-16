#pragma once

#include <io/manager.hpp>
#include <lsm/lsm.hpp>
#include <wal/writer.hpp>

namespace structuredb::server::table {

class Table {
public:
  using Ptr = std::shared_ptr<Table>;

  explicit Table(io::Manager& io_manager, const std::string& base_dir);

  void StartWal(wal::Writer::Ptr wal_writer);

  Awaitable<void> Upsert(const int64_t tx, const std::string& key, const std::string& value);

  Awaitable<std::optional<std::string>> Lookup(const int64_t tx, const std::string& key);
private:
  lsm::Lsm lsm_;
  wal::Writer::Ptr wal_writer_{nullptr};
};

}
