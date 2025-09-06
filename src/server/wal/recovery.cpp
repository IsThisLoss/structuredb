#include "recovery.hpp"

#include <spdlog/spdlog.h>

#include <io/exceptions.hpp>
#include <sdb/buffer_reader.hpp>
#include <wal/events/event.hpp>
#include <wal/events/io.hpp>
#include <wal/wal.hpp>

namespace structuredb::server::wal {

Awaitable<void> Recover(
    io::Manager& io_manager,
    const std::string& wal_path, 
    database::Database& db
) {
  SPDLOG_INFO("Starting recovery...");

  auto wal_file_reader = co_await io_manager.CreateFileReader(wal_path);
  std::vector<char> page_buffer;
  while (true) {
    try {
      page_buffer.resize(kWalPageSize, 0);
      size_t read_bytes = co_await wal_file_reader->Read(page_buffer.data(), page_buffer.size());
      if (read_bytes == 0) {
        SPDLOG_INFO("Reached end of wal file");
        break;
      }
      // last page may be not full
      page_buffer.resize(read_bytes);

      sdb::BufferReader wal_page_reader{std::move(page_buffer)};
      page_buffer.clear();

      while (wal_page_reader.HasMore()) {
        auto event = ParseEvent(wal_page_reader);
        co_await event->Apply(db);
      }
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
