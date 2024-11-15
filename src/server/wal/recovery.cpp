#include "recovery.hpp"

#include "events/io.hpp"

namespace structuredb::server::wal {

Awaitable<void> Recover(io::Manager& io_manager, const std::string& path, lsm::Lsm& lsm) {
  if (!co_await io_manager.IsFileExists(path)) {
    co_return;
  }

  sdb::Reader reader{io_manager.CreateFileReader(path)};
  auto event = co_await ParseEvent(reader);
  co_await event->Apply(lsm);
}

}

