#include "recovery.hpp"

#include <iostream>

#include <io/exceptions.hpp>

#include "events/io.hpp"

namespace structuredb::server::wal {

Awaitable<void> Recover(io::Manager& io_manager, const std::string& path, database::Database& db) {
  if (!co_await io_manager.IsFileExists(path)) {
    co_return;
  }

  sdb::Reader reader{io_manager.CreateFileReader(path)};
  while (true) {
    try {
      auto event = co_await ParseEvent(reader);
      co_await event->Apply(db);
    } catch (const io::EndOfFile& e) {
      std::cerr << "End of file reached\n";
      break;
    }
  }
}

}

