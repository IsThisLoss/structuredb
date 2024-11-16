#include "writer.hpp"

#include <iostream>

#include "events/io.hpp"

namespace structuredb::server::wal {

Writer::Writer(sdb::Writer&& wal_writer, sdb::Writer&& control_writer)
  : wal_writer_(std::move(wal_writer))
  , control_writer_(std::move(control_writer))
{}

Awaitable<void> Writer::Write(Event::Ptr event) {
  co_await FlushEvent(wal_writer_, event);
  co_await wal_writer_.FSync();
}

Awaitable<void> Writer::SetPersistedTx(int64_t tx) {
  std::cerr << "Trying to set pers tx: " << tx << std::endl;
  co_await control_writer_.WriteInt(tx);
  co_await control_writer_.Rewind();
  std::cerr << "Here: " << tx << std::endl;
  co_await control_writer_.FSync();
}

Awaitable<Writer::Ptr> Open(io::Manager& io_manager, const std::string& wal_path, const std::string& control_path) {
  std::cerr << "Opening wal...\n";
  auto wal_file_writer = co_await io_manager.CreateFileWriter(wal_path, /*append=*/ true);
  sdb::Writer wal_writer{std::move(wal_file_writer)};

  auto control_file_writer = co_await io_manager.CreateFileWriter(control_path);
  sdb::Writer control_writer{std::move(control_file_writer)};

  std::cerr << "Wal opened\n";
  co_return std::make_shared<Writer>(std::move(wal_writer), std::move(control_writer));
}

}
