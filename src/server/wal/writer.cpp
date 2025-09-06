#include "writer.hpp"

#include <spdlog/spdlog.h>

#include <sdb/buffer_writer.hpp>

#include "events/io.hpp"
#include "wal.hpp"

namespace structuredb::server::wal {

Writer::Writer(io::FileWriter::Ptr&& wal_writer)
  : wal_writer_(std::move(wal_writer))
{
  assert(wal_writer_ != nullptr);
}

Awaitable<void> Writer::Write(Event::Ptr event) {
  sdb::BufferWriter buffer_writer;
  FlushEvent(buffer_writer, event);
  const auto data = std::move(buffer_writer).Extract();

  if (current_page_size_ + static_cast<int64_t>(data.size()) > kWalPageSize) {
    co_await FlushPage();
  }

  co_await wal_writer_->Write(data.data(), data.size());
  current_page_size_ += static_cast<int64_t>(data.size());

  co_await wal_writer_->FSync();
}

Awaitable<void> Writer::FlushPage() {
  if (current_page_size_ == 0) {
    SPDLOG_WARN("Attempt to flush empty page");
    co_return;
  }
  if (current_page_size_ < kWalPageSize) {
    std::vector<char> padding(kWalPageSize - current_page_size_, 0);
    co_await wal_writer_->Write(padding.data(), padding.size());
  }
  current_page_size_ = 0;
}

Awaitable<Writer::Ptr> Open(io::Manager& io_manager, const std::string& wal_path) {
  SPDLOG_INFO("Opening wal...");
  auto wal_file_writer = co_await io_manager.CreateFileWriter(wal_path, /*append=*/ true);
  SPDLOG_INFO("Wal opened");
  co_return std::make_shared<Writer>(std::move(wal_file_writer));
}

}
