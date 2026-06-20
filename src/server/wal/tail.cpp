#include "tail.hpp"

#include <algorithm>
#include <cstdint>
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

Awaitable<std::vector<WalPageData>> CollectStablePages(
    io::Manager& io_manager,
    const std::string& wal_dir_path,
    Position from
) {
  std::vector<WalPageData> result;

  auto segments = co_await io_manager.ListDirectory(wal_dir_path);
  if (segments.size() <= 1) {
    // only the in-progress segment (if any) exists; nothing stable to ship
    co_return result;
  }
  std::ranges::sort(segments);

  // skip the last segment: it is the one currently being written
  for (size_t i = 0; i + 1 < segments.size(); ++i) {
    const auto segment_no = GetSegmentNoFromName(segments[i]);
    if (segment_no < from.segment_no) {
      continue;
    }
    const auto segment_path = wal_dir_path + "/" + segments[i];
    co_await ReadSegmentPages(io_manager, segment_path, segment_no, from, result);
  }

  co_return result;
}

}
