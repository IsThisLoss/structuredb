#include "reader.hpp"

#include <io/exceptions.hpp>
#include <wal/segment.hpp>
#include <wal/wal.hpp>

namespace structuredb::server::wal {

Reader::Reader(io::Manager& io_manager, ReaderStrategy::Ptr strategy)
  : io_manager_{io_manager}
  , strategy_{std::move(strategy)}
{}

Awaitable<void> Reader::Read(const std::string& wal_dir_path) {
  SPDLOG_INFO("Starting reading wal...");

  auto segments = co_await io_manager_.ListDirectory(wal_dir_path);
  if (segments.empty()) {
    SPDLOG_INFO("No wal file found, nothing to read");
    co_return;
  }
  std::ranges::sort(segments);

  for (const auto& segment : segments) {
    const auto wal_segment_path = wal_dir_path + "/" + segment;
    SPDLOG_INFO("Reading wal segment: {}", wal_segment_path);
    int64_t segment_no = GetSegmentNoFromName(segment);
    co_await ReadSegment(wal_segment_path, segment_no);
  }

  SPDLOG_INFO("Reading wal done");
}

Awaitable<void> Reader::ReadSegment(const std::string& wal_segment_path, int64_t segment_no) {
  auto wal_file_reader = co_await io_manager_.CreateFileReader(wal_segment_path);
  int64_t page_no = 0;
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

      co_await strategy_->OnPage({segment_no, page_no}, std::move(page_buffer));

      page_buffer.clear();
    } catch (const io::EndOfFile& e) {
      SPDLOG_INFO("Reached end of wal file: {}", e.what());
      break;
    } catch (const std::exception& e) {
      SPDLOG_ERROR("Exception while recover from wal file: {}", e.what());
      break;
    }
  }
}

}
