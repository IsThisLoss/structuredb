#pragma once

#include <memory>

#include <io/manager.hpp>

namespace structuredb::server::wal {

struct Position {
  int64_t segment_no{0};
  int64_t page_no{0};
};

class ReaderStrategy {
public:
  using Ptr = std::shared_ptr<ReaderStrategy>;

  virtual Awaitable<void> OnPage(Position pos, std::vector<char> page_buffer) = 0;

  virtual ~ReaderStrategy() = default;
};

class Reader {
public:
  explicit Reader(io::Manager& io_manager, ReaderStrategy::Ptr strategy);

  Awaitable<void> Read(const std::string& wal_dir_path);

private:
  io::Manager& io_manager_;
  ReaderStrategy::Ptr strategy_;

  Awaitable<void> ReadSegment(const std::string& wal_segment_path, int64_t segment_no);
};

}
