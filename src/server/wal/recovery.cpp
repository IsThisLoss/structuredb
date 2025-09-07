#include "recovery.hpp"

#include <spdlog/spdlog.h>

#include <io/exceptions.hpp>
#include <sdb/buffer_reader.hpp>
#include <wal/events/event.hpp>
#include <wal/events/io.hpp>
#include <wal/reader.hpp>

namespace structuredb::server::wal {

namespace {

class RecoveryStrategy : public ReaderStrategy {
public:
  explicit RecoveryStrategy(database::Database& db)
    : db_{db}
  {}

  Awaitable<void> OnPage(Position pos, std::vector<char> page_buffer) override {
    SPDLOG_INFO("Recovering from segment {}, page {}", pos.segment_no, pos.page_no);
    sdb::BufferReader wal_page_reader{std::move(page_buffer)};
    while (wal_page_reader.HasMore()) {
      auto event = ParseEvent(wal_page_reader);
      co_await event->Apply(db_);
    }
  }

private:
  database::Database& db_;
};

}

Awaitable<void> Recover(
    io::Manager& io_manager,
    const std::string& wal_dir_path, 
    database::Database& db
) {
  SPDLOG_INFO("Starting recovery...");
  auto strategy = std::make_shared<RecoveryStrategy>(db);
  Reader reader{io_manager, std::move(strategy)};
  co_await reader.Read(wal_dir_path);
  SPDLOG_INFO("Recovery done");
}

}
