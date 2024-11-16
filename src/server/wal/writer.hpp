#pragma once

#include <sdb/writer.hpp>
#include <io/manager.hpp>

#include "events/event.hpp"

namespace structuredb::server::wal {

/// @brief writes WAL records
class Writer {
public:
  using Ptr = std::shared_ptr<Writer>;

  explicit Writer(sdb::Writer&& wal_writer, sdb::Writer&& control_writer);

  Awaitable<void> Write(Event::Ptr event);

  Awaitable<void> SetPersistedTx(int64_t tx);
private:
  sdb::Writer wal_writer_;
  sdb::Writer control_writer_;
};

Awaitable<Writer::Ptr> Open(io::Manager& io_manager, const std::string& wal_path, const std::string& control_path);

}
