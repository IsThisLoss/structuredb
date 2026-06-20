#include "cleaner.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <sdb/buffer_reader.hpp>
#include <wal/events/event.hpp>
#include <wal/events/io.hpp>
#include <wal/reader.hpp>
#include <spdlog/spdlog.h>

namespace structuredb::server::wal {

namespace {

class CleanerStrategy : public ReaderStrategy {
public:
  explicit CleanerStrategy(database::Database& db)
    : db_{db}
  {}

  Awaitable<void> OnPage(Position pos, std::vector<char> page_buffer) override {
    SPDLOG_INFO("Checking persistance of segment {}, page {}", pos.segment_no, pos.page_no);
    sdb::BufferReader wal_page_reader{std::move(page_buffer)};
    while (wal_page_reader.HasMore()) {
      auto event = ParseEvent(wal_page_reader);
      const bool is_persistent = co_await event->IsPersistent(db_);
      AccumutaePersistence(pos.segment_no, is_persistent);
    }
  }

  Awaitable<void> DeleteObsoleteSegments(
      io::Manager& io_manager,
      const std::string& wal_dir_path,
      std::optional<int64_t> retain_from_segment
  ) {
    for (const auto& [segment_no, is_persistent] : segment_persistence_) {
      if (!is_persistent) {
        continue;
      }
      if (retain_from_segment.has_value() && segment_no >= retain_from_segment.value()) {
        // still needed by a connected replication follower
        continue;
      }
      const auto segment_file_name = std::format("{}/{:04d}.wal.sdb", wal_dir_path, segment_no);
      SPDLOG_INFO("Deleting obsolete wal segment file: {}", segment_file_name);
      co_await io_manager.Remove(segment_file_name);
    }
  }

private:
  database::Database& db_;
  std::unordered_map<int64_t, bool> segment_persistence_;

  void AccumutaePersistence(int64_t segment_no, bool is_persistent) {
    auto it = segment_persistence_.find(segment_no);
    if (it == segment_persistence_.end()) {
      segment_persistence_.emplace(segment_no, is_persistent);
      return;
    }
    it->second = it->second && is_persistent;
  }
};

}

Awaitable<void> Clean(
    io::Manager& io_manager,
    const std::string& wal_dir_path,
    database::Database& db,
    std::optional<int64_t> retain_from_segment
) {
  SPDLOG_INFO("Starting cleaning wal files...");

  auto strategy = std::make_shared<CleanerStrategy>(db);
  Reader reader{io_manager, strategy};
  co_await reader.Read(wal_dir_path);

  co_await strategy->DeleteObsoleteSegments(io_manager, wal_dir_path, retain_from_segment);

  SPDLOG_INFO("Cleaning wal files done");
}

}
