#include "writer.hpp"

#include "events/io.hpp"

namespace structuredb::server::wal {

Writer::Writer(sdb::Writer&& wal_writer)
  : wal_writer_(std::move(wal_writer))
{}

Awaitable<void> Writer::Write(Event::Ptr event) {
  co_await FlushEvent(wal_writer_, event);
  co_await wal_writer_.FSync();
}

Awaitable<Writer> Open(io::Manager& io_manager, const std::string& wal_path) {
  auto file_writer = io_manager.CreateFileWriter(wal_path);
  sdb::Writer sdb_writer{std::move(file_writer)};
  co_return Writer{std::move(sdb_writer)};
}

}
