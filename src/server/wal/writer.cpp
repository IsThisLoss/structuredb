#include "writer.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include <spdlog/spdlog.h>

#include <sdb/buffer_writer.hpp>

#include <wal/events/io.hpp>
#include <wal/segment.hpp>
#include <wal/wal.hpp>

namespace structuredb::server::wal {

Awaitable<Writer::Ptr> Writer::Open(io::Manager& io_manager, const std::string& wal_dir_path) {
  auto result = std::make_shared<Writer>(io_manager, wal_dir_path);
  co_await result->Init();
  co_return result;
}

Writer::Writer(io::Manager& io_manager, std::string wal_dir_path)
  : io_manager_{io_manager}
  , wal_dir_path_{std::move(wal_dir_path)}
{}

Awaitable<void> Writer::Init() {
  co_await io_manager_.CreateDirectory(wal_dir_path_);

  auto segments = co_await io_manager_.ListDirectory(wal_dir_path_);
  std::ranges::sort(segments);
  if (!segments.empty()) {
    current_segment_no_ = GetSegmentNoFromName(segments.back()) + 1;
  }

  co_await OpenSegment();
}

Awaitable<void> Writer::Write(Event::Ptr event) {
  assert(current_segment_writer_ != nullptr);

  sdb::BufferWriter buffer_writer;
  FlushEvent(buffer_writer, event);
  const auto data = std::move(buffer_writer).Extract();

  if (current_page_size_ + static_cast<int64_t>(data.size()) > kWalPageSize) {
    co_await FlushPage();
  }

  co_await current_segment_writer_->Write(data.data(), data.size());
  current_page_size_ += static_cast<int64_t>(data.size());

  co_await current_segment_writer_->FSync();
}

Awaitable<void> Writer::FlushPage() {
  if (current_page_size_ == 0) {
    SPDLOG_WARN("Attempt to flush empty page");
    co_return;
  }
  if (current_page_size_ < kWalPageSize) {
    std::vector<char> padding(kWalPageSize - current_page_size_, 0);
    co_await current_segment_writer_->Write(padding.data(), padding.size());
  }
  current_page_size_ = 0;
  pages_in_current_segment_++;

  if (pages_in_current_segment_ >= kMaxPagesInSegment) {
    co_await FlushSegment();
  }
}

Awaitable<void> Writer::FlushSegment() {
  if (current_segment_writer_ == nullptr) {
    SPDLOG_WARN("Attempt to flush unopened segment");
    co_return;
  }

  current_segment_no_++;
  co_await OpenSegment();
}

Awaitable<void> Writer::OpenSegment() {
  pages_in_current_segment_ = 0;
  const auto segment_path = std::format("{}/{:04d}.wal.sdb", wal_dir_path_, current_segment_no_);
  current_segment_writer_ = co_await io_manager_.CreateFileWriter(segment_path, true);
}

}
