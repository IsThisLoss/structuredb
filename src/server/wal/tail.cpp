#include "tail.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include <io/exceptions.hpp>
#include <wal/segment.hpp>
#include <wal/wal.hpp>

namespace structuredb::server::wal {

bool operator<(const Position& lhs, const Position& rhs) {
  if (lhs.segment_no != rhs.segment_no) {
    return lhs.segment_no < rhs.segment_no;
  }
  return lhs.page_no < rhs.page_no;
}

namespace {

Awaitable<void> ReadSegmentPages(
    io::Manager& io_manager,
    const std::string& segment_path,
    int64_t segment_no,
    Position from,
    std::vector<WalPageData>& out
) {
  auto reader = co_await io_manager.CreateFileReader(segment_path);
  int64_t page_no = 0;
  while (true) {
    std::vector<char> page_buffer(kWalPageSize, 0);
    try {
      const size_t read_bytes = co_await reader->Read(page_buffer.data(), page_buffer.size());
      if (read_bytes == 0) {
        break;
      }
      page_buffer.resize(read_bytes);
    } catch (const io::EndOfFile&) {
      break;
    }

    const Position pos{.segment_no = segment_no, .page_no = page_no};
    if (!(pos < from)) {
      out.push_back(WalPageData{.position = pos, .data = std::move(page_buffer)});
    }
    ++page_no;
  }
}

}

Awaitable<std::vector<WalPageData>> CollectPagesFrom(
    io::Manager& io_manager,
    const std::string& wal_dir_path,
    Position from
) {
  std::vector<WalPageData> result;

  auto segments = co_await io_manager.ListDirectory(wal_dir_path);
  std::ranges::sort(segments);

  for (const auto& segment : segments) {
    const auto segment_no = GetSegmentNoFromName(segment);
    if (segment_no < from.segment_no) {
      continue;
    }
    const auto segment_path = wal_dir_path + "/" + segment;
    co_await ReadSegmentPages(io_manager, segment_path, segment_no, from, result);
  }

  co_return result;
}

Awaitable<int64_t> NextSegmentToFetch(
    io::Manager& io_manager,
    const std::string& wal_dir_path
) {
  int64_t segment_no = 0;
  while (true) {
    const auto path = std::format("{}/{:04d}.wal.sdb", wal_dir_path, segment_no);
    const auto size = co_await io_manager.FileSize(path);
    if (size < 0 || size < kWalPageSize) {
      co_return segment_no;
    }
    ++segment_no;
  }
}

Awaitable<void> WriteReceivedPage(
    io::Manager& io_manager,
    const std::string& wal_dir_path,
    int64_t segment_no,
    const std::string& data
) {
  const auto path = std::format("{}/{:04d}.wal.sdb", wal_dir_path, segment_no);
  auto writer = co_await io_manager.CreateFileWriter(path, /*append=*/false);
  co_await writer->Write(data.data(), data.size());
  co_await writer->FSync();
}

}
