#pragma once

#include <sdb/writer.hpp>
#include <io/manager.hpp>

#include "events/event.hpp"

namespace structuredb::server::wal {

/// @brief writes WAL records
class Writer {
public:
  using Ptr = std::shared_ptr<Writer>;

  explicit Writer(sdb::Writer&& wal_writer);

  Awaitable<void> Write(Event::Ptr event);
private:
  sdb::Writer wal_writer_;
};

Awaitable<Writer::Ptr> Open(io::Manager& io_manager, const std::string& wal_path);

}
