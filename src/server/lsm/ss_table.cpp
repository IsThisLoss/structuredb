#include "ss_table.hpp"

namespace structuredb::server::lsm {

SSTable::SSTable(io::FileReader&& file_reader)
  : file_reader_{std::move(file_reader)}
{}

Awaitable<void> SSTable::Init() {
  co_await file_reader_.Read(reinterpret_cast<char*>(&header_), sizeof(header_));
}

Awaitable<std::optional<std::string>> SSTable::Get(const std::string& key) {
  // binary search
  size_t lo = 0;
  size_t hi = header_.page_count;

  while (lo < hi) {
    size_t mid = lo + (hi - lo) / 2;
    co_await SetFilePos(header_.page_size * mid);
    // TODO use lru cache
    auto page = co_await disk::Page::Load(file_reader_);
    if (key < page.MinKey()) {
      hi = mid;
    }
    else if (page.MaxKey() < key) {
      hi = mid + 1;
    } else {
      co_return page.Find(key);
    }
  }

  co_return std::nullopt;
}

Awaitable<void> SSTable::SetFilePos(size_t pos) {
  co_await file_reader_.Seek(sizeof(header_) + pos);
}

}
