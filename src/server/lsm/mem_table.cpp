#include "mem_table.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <spdlog/spdlog.h>

#include <utils/find.hpp>

#include "disk/ss_table_builder.hpp"
#include "iterators/mem_table_iterator.hpp"

namespace structuredb::server::lsm {

void MemTable::Put(Record&& record) {
  impl_.insert(std::move(record));
}

size_t MemTable::Size() const {
  return impl_.size();

}

Awaitable<SSTable> MemTable::Flush(io::Manager& io_manager, const std::string& file_path, int64_t page_size) const {
  SPDLOG_INFO("MemTable flush started");

  // This bock is important because
  // file_writer closes file in destructor
  {
    auto file_writer = co_await io_manager.CreateFileWriter(file_path);
    auto builder = co_await disk::SSTableBuilder::Create(file_writer, page_size);
    for (const auto& record : impl_) {
      co_await builder.Add(record);
    }
    co_await std::move(builder).Finish();
    SPDLOG_INFO("SSTableBuilder finished");
  }

  auto file_reader = co_await io_manager.CreateFileReader(file_path);
  auto ss_table = co_await SSTable::Create(std::move(file_reader));
  co_return ss_table;
}

Iterator::Ptr MemTable::Scan(const ScanRange& range) const {
  return std::make_shared<MemTableIterator>(*this, range);
}

}
