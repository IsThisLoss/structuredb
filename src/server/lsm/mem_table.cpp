#include "mem_table.hpp"

#include <iostream>

#include <utils/find.hpp>

#include "disk/ss_table_builder.hpp"

namespace structuredb::server::lsm {

void MemTable::Put(Record&& record) {
  impl_.insert(std::move(record));
}

bool MemTable::Scan(const std::string& key, const RecordConsumer& consume) const {
  auto it = impl_.lower_bound(Record{key});
  for (; it != impl_.end() && it->key == key; it++) {
    if (consume(it->value)) {
      return true;
    }
  }
  return false;
}

void MemTable::ScanValues(const RecordConsumer& consume) const {
  for (const auto& [key, seq_no, value] : impl_) {
    consume(value);
  }
}

size_t MemTable::Size() const {
  return impl_.size();

}

Awaitable<SSTable> MemTable::Flush(io::Manager& io_manager, const std::string& file_path) const {
  constexpr static const int64_t kPageSize = 512;

  std::cerr << "MemTable flush start\n";

  // This bock is important because
  // file_writer closes file in destructor
  {
    auto file_writer = co_await io_manager.CreateFileWriter(file_path);
    auto builder = co_await disk::SSTableBuilder::Create(file_writer, kPageSize);
    for (const auto& record : impl_) {
      co_await builder.Add(record);
    }
    co_await std::move(builder).Finish();
    std::cerr << "SSTableBuilder finished\n";
  }

  auto file_reader = co_await io_manager.CreateFileReader(file_path);
  auto ss_table = co_await SSTable::Create(std::move(file_reader));
  co_return ss_table;
}

}
