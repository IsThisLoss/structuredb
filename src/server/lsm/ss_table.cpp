#include "ss_table.hpp"

#include <iostream>

#include <utils/find.hpp>

namespace structuredb::server::lsm {

Awaitable<SSTable> SSTable::Create(io::FileReader::Ptr file_reader) {
  SSTable result{std::move(file_reader)};
  co_await result.Init();
  co_return result;
}

SSTable::SSTable(io::FileReader::Ptr file_reader)
  : file_reader_{std::move(file_reader)}
{}

Awaitable<void> SSTable::Init() {
  header_size_ = disk::SSTableHeader::EstimateSize(header_);

  std::vector<char> buffer(header_size_);
  co_await file_reader_->Read(buffer.data(), buffer.size());
  sdb::BufferReader buffer_reader{std::move(buffer)};

  header_ = co_await disk::SSTableHeader::Load(buffer_reader);
  std::cerr << "Initialize ss table: " << header_.page_count << std::endl;
}

Awaitable<bool> SSTable::Scan(const std::string& key, const RecordConsumer& consume) {
  // binary search
  size_t lo = 0;
  size_t hi = header_.page_count;

  std::cerr << key << std::endl;
  while (lo < hi) {
    size_t mid = lo + (hi - lo) / 2;
    // std::cerr << "Read page #" << mid << std::endl;
    auto page = co_await GetPage(mid);

    if (key <= page.MaxKey()) {
      hi = mid;
    } else {
      lo = mid + 1;
    }
  }

  if (lo >= header_.page_count) {
    co_return false;
  }

  for (size_t i = lo; i < header_.page_count; i++) {
    auto page = co_await GetPage(i);
    if (key > page.MaxKey()) {
      break;
    }

    auto values = page.Find(key);
    for (const auto& value : values) {
      if (consume(value)) {
        co_return true;
      }
    }
  }
  co_return false;
}


Sequence SSTable::GetMaxSeqNo() const {
  return header_.max_seq_no;
}

Awaitable<disk::Page> SSTable::GetPage(int64_t page_num) {
  assert(page_num < header_.page_count);
  co_await file_reader_->Seek(header_size_ + page_num * header_.page_size);

  const auto* cached = utils::FindOrNullptr(page_cache_, page_num);
  if (cached) {
    co_return *cached;
  }

  std::vector<char> buffer(header_.page_size);
  co_await file_reader_->Read(buffer.data(), buffer.size());
  sdb::BufferReader buffer_reader{std::move(buffer)};
  co_return page_cache_[page_num] = co_await disk::Page::Load(buffer_reader);
}

}
