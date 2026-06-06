#include "ss_table.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

#include <sdb/buffer_reader.hpp>

#include "iterators/ss_table_iterator.hpp"

namespace structuredb::server::lsm {

Awaitable<SSTable> SSTable::Create(io::FileReader::Ptr file_reader, size_t page_cache_capacity) {
  SSTable result{std::move(file_reader), page_cache_capacity};
  co_await result.Init();
  co_return result;
}

SSTable::SSTable(io::FileReader::Ptr file_reader, size_t page_cache_capacity)
  : file_reader_{std::move(file_reader)}
  , page_cache_{page_cache_capacity}
{}

Awaitable<void> SSTable::Init() {
  SPDLOG_INFO("Initialize SSTable from file {}", file_reader_->Path());
  header_size_ = disk::SSTableHeader::SdbSize();

  std::vector<char> buffer(header_size_);
  co_await file_reader_->Read(buffer.data(), buffer.size());
  sdb::BufferReader buffer_reader{std::move(buffer)};
  Read(buffer_reader, header_);
  SPDLOG_INFO("SSTable was initialized, pages = {}, page_size = {}", header_.page_count, header_.page_size);
}

Awaitable<Iterator::Ptr> SSTable::Scan(const ScanRange& range) {
  co_return std::make_shared<SSTableIterator>(co_await SSTableIterator::Create(*this, range));
}

Awaitable<int64_t> SSTable::LowerBound(const std::string& key) {
  // binary search
  int64_t lo = 0;
  int64_t hi = header_.page_count;

  while (lo < hi) {
    int64_t mid = lo + (hi - lo) / 2;
    auto page = co_await GetPage(mid);

    if (key <= page->MaxKey()) {
      hi = mid;
    } else {
      lo = mid + 1;
    }
  }

  co_return lo;
}

Sequence SSTable::GetMaxSeqNo() const {
  return header_.max_seq_no;
}

Awaitable<disk::Page::Ptr> SSTable::GetPage(int64_t page_num) {
  assert(page_num < header_.page_count);

  const auto key = static_cast<size_t>(page_num);
  if (auto* cached = page_cache_.Get(key)) {
    co_return *cached;
  }

  co_await file_reader_->Seek(header_size_ + page_num * header_.page_size);
  std::vector<char> buffer(header_.page_size);
  co_await file_reader_->Read(buffer.data(), buffer.size());

  auto page = disk::Page::Load(std::move(buffer));
  page_cache_.Put(key, page);
  co_return page;
}

const std::string& SSTable::GetFilePath() const {
  return file_reader_->Path();
}

}
