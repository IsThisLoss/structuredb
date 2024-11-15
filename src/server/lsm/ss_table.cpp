#include "ss_table.hpp"

#include <iostream>

namespace structuredb::server::lsm {

SSTable::SSTable(io::FileReader&& file_reader)
  : file_reader_{std::move(file_reader)}
{}

Awaitable<void> SSTable::Init() {
  co_await file_reader_.Read(reinterpret_cast<char*>(&header_.page_size), sizeof(int64_t));
  co_await file_reader_.Read(reinterpret_cast<char*>(&header_.page_count), sizeof(int64_t));
  int64_t x=0;
  std::cerr << "SSTable initialized: " << header_.page_size << " " << header_.page_count << " x " << x << std::endl;
}

Awaitable<std::optional<std::string>> SSTable::Get(const std::string& key) {
  // binary search
  size_t lo = 0;
  size_t hi = header_.page_count;

  co_await SetFilePos(0);
  for (int i = 0; i < header_.page_count; i++) {
    auto page = co_await disk::Page::Load(file_reader_);
    auto value = page.Find(key);
    if (value.has_value()) {
      co_return value;
    }
  }
  co_return std::nullopt;

  /*
  while (lo < hi) {
    size_t mid = lo + (hi - lo) / 2;
    co_await SetFilePos(header_.page_size * mid);
    // TODO use lru cache
    auto page = co_await disk::Page::Load(file_reader_);
    std::cerr << "Loaded page: " << page.MinKey() << " " << page.MaxKey() << std::endl;
    if (key < page.MinKey()) {
      hi = mid;
    }
    else if (page.MaxKey() < key) {
      hi = mid + 1;
    } else {
      co_return page.Find(key);
    }
  }
  */

  co_return std::nullopt;
}

Awaitable<void> SSTable::SetFilePos(size_t pos) {
  co_await file_reader_.Seek(2*sizeof(int64_t) + pos);
}

}
