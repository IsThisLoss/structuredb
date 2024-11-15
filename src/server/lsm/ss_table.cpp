#include "ss_table.hpp"

#include <iostream>

namespace structuredb::server::lsm {

Awaitable<SSTable> SSTable::Create(io::FileReader&& file_reader) {
  SSTable result{std::move(file_reader)};
  co_await result.Init();
  co_return result;
}

SSTable::SSTable(io::FileReader&& file_reader)
  : file_reader_{std::move(file_reader)}
  , sdb_reader_{file_reader_}
{}

Awaitable<void> SSTable::Init() {
  header_ = co_await disk::SSTableHeader::Load(sdb_reader_);
  header_size_ = disk::SSTableHeader::EstimateSize(header_);
  std::cerr << "Initialize ss table: " << header_.page_count << std::endl;
}

Awaitable<std::optional<std::string>> SSTable::Get(const std::string& key) {
  // binary search
  size_t lo = 0;
  size_t hi = header_.page_count;

  co_await SetFilePos(0);
  for (int i = 0; i < header_.page_count; i++) {
    auto page = co_await disk::Page::Load(sdb_reader_);
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
  co_await file_reader_.Seek(header_size_ + pos);
}

}
