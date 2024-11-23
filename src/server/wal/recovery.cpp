#include "recovery.hpp"

#include <spdlog/spdlog.h>

#include <io/exceptions.hpp>

#include "events/io.hpp"

namespace structuredb::server::wal {

Awaitable<void> Recover(
    io::Manager& io_manager,
    const std::string& wal_path, 
    const std::string& control_path,
    database::Database& db
) {
  SPDLOG_INFO("Starting recovery...");

  sdb::Reader wal_reader{co_await io_manager.CreateFileReader(wal_path)};
  while (true) {
    try {
      auto event = co_await ParseEvent(wal_reader);
      co_await event->Apply(db);
    } catch (const io::EndOfFile& e) {
      SPDLOG_INFO("Reached end of wal file: {}", e.what());
      break;
    } catch (const std::exception& e) {
      SPDLOG_ERROR("Exception while recover from wal file: {}", e.what());
      break;
    }
  }

  SPDLOG_INFO("Recovery done");
}

}

