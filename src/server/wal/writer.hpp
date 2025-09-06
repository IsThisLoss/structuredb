#pragma once

#include <io/manager.hpp>

#include "events/event.hpp"

namespace structuredb::server::wal {

/// @brief writes WAL records
class Writer {
public:
  using Ptr = std::shared_ptr<Writer>;

  explicit Writer(io::FileWriter::Ptr&& wal_writer);

  Awaitable<void> Write(Event::Ptr event);

private:
  io::FileWriter::Ptr wal_writer_;

  int64_t current_page_size_ = 0;

  Awaitable<void> FlushPage();
};

Awaitable<Writer::Ptr> Open(io::Manager& io_manager, const std::string& wal_path);

}
