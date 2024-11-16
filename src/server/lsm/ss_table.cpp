#include "ss_table.hpp"

#include <iostream>

namespace structuredb::server::lsm {

Awaitable<SSTable> SSTable::Create(io::FileReader::Ptr file_reader) {
  SSTable result{std::move(file_reader)};
  co_await result.Init();
  co_return result;
}

SSTable::SSTable(io::FileReader::Ptr file_reader)
  : sdb_reader_{std::move(file_reader)}
{}

Awaitable<void> SSTable::Init() {
  header_ = co_await disk::SSTableHeader::Load(sdb_reader_);
  header_size_ = disk::SSTableHeader::EstimateSize(header_);
  std::cerr << "Initialize ss table: " << header_.page_count << std::endl;
}

Awaitable<void> SSTable::Get(const std::string& key, const RecordConsumer& consume) {
  // binary search
  size_t lo = 0;
  size_t hi = header_.page_count;

  co_await SetFilePos(0);
  for (int i = 0; i < header_.page_count; i++) {
    auto page = co_await disk::Page::Load(sdb_reader_);
    auto value = page.Find(key);
    if (value.has_value()) {
      consume(value.value());
    }
  }

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
}

Awaitable<void> SSTable::SetFilePos(size_t pos) {
  co_await sdb_reader_.Seek(header_size_ + pos);
}

}
