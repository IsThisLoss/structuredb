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

  while (lo < hi) {
    size_t mid = lo + (hi - lo) / 2;
    std::cerr << "Read page #" << mid << std::endl;
    auto buffer = co_await GetPage(mid);
    auto page = co_await disk::Page::Load(buffer);

    if (key < page.MinKey()) {
      hi = mid;
    }
    else if (page.MaxKey() < key) {
      lo = mid + 1;
    } else {
      auto value = page.Find(key);
      if (value.has_value()) {
        if (consume(value.value())) {
          co_return true;
        }
        break;
      }
    }
  }
  co_return false;
}


Sequence SSTable::GetMaxSeqNo() const {
  return header_.max_seq_no;
}

Awaitable<sdb::BufferReader> SSTable::GetPage(int64_t page_num) {
  assert(page_num < header_.page_count);
  co_await file_reader_->Seek(header_size_ + page_num * header_.page_size);

  const auto* cached_buffer = utils::FindOrNullptr(page_cache_, page_num);
  if (cached_buffer) {
    co_return sdb::BufferReader{std::move(*cached_buffer)};
  }

  std::cerr << "Cache miss\n";
  std::vector<char> buffer(header_.page_size);
  co_await file_reader_->Read(buffer.data(), buffer.size());
  page_cache_[page_num] = buffer;
  co_return sdb::BufferReader{std::move(buffer)};
}

}
