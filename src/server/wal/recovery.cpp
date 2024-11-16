#include "recovery.hpp"

#include <iostream>

#include <io/exceptions.hpp>

#include "events/io.hpp"

namespace structuredb::server::wal {

Awaitable<void> Recover(
    io::Manager& io_manager,
    const std::string& wal_path, 
    const std::string& control_path,
    database::Database& db
) {
  std::cerr << "Starting recovery...\n";
  try {
      sdb::Reader control_reader{co_await io_manager.CreateFileReader(control_path)};
      const int64_t max_tx = co_await control_reader.ReadInt();
      db.SetTx(max_tx);
  } catch (const std::exception& e) {
    std::cerr << "Failed to read control file\n";
  }

  sdb::Reader wal_reader{co_await io_manager.CreateFileReader(wal_path)};
  while (true) {
    try {
      auto event = co_await ParseEvent(wal_reader);
      co_await event->Apply(db);
    } catch (const io::EndOfFile& e) {
      std::cerr << "End of file reached: " << e.what() << "\n";
      break;
    } catch (const std::exception& e) {
      std::cerr << "Error while reading wal: " << e.what() << "\n";
      break;
    }
  }
  std::cerr << "Recovery done\n";
}

}
