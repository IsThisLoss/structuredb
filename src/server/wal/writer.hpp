#pragma once

#include <memory>
#include <string>
#include <io/manager.hpp>

#include "events/event.hpp"

namespace structuredb::server::wal {

/// @brief writes WAL records
class Writer {
public:
  using Ptr = std::shared_ptr<Writer>;

  static Awaitable<Writer::Ptr> Open(io::Manager& io_manager, const std::string& wal_dir_path);

  explicit Writer(io::Manager& io_manager, std::string wal_dir_path);

  Awaitable<void> Write(Event::Ptr event);

private:
  io::Manager& io_manager_;
  std::string wal_dir_path_;
  io::FileWriter::Ptr current_segment_writer_;

  int64_t current_page_size_ = 0;
  int64_t pages_in_current_segment_ = 0;
  int64_t current_segment_no_ = 0;

  Awaitable<void> Init();

  Awaitable<void> FlushPage();

  Awaitable<void> FlushSegment();

  Awaitable<void> OpenSegment();
};


}
