#include "mem_table.hpp"

#include <iostream>

#include <utils/find.hpp>

#include "disk/ss_table_builder.hpp"

namespace structuredb::server::lsm {

void MemTable::Put(const std::string& key, const std::string& value) {
  impl_.insert_or_assign(key, value);
}

std::optional<std::string> MemTable::Get(const std::string& key) const {
  const auto* value = utils::FindOrNullptr(impl_, key);
  if (value) {
    return std::make_optional(*value);
  }
  return std::nullopt;
}

size_t MemTable::Size() const {
  return impl_.size();

}

Awaitable<SSTable> MemTable::Flush(io::Manager& io_manager, const std::string& file_path) const {
  constexpr static const int64_t kPageSize = 8;

  std::cerr << "MemTable flush start\n";

  // This bock is important because
  // file_writer closes file in destructor
  {
    auto file_writer = io_manager.CreateFileWriter(file_path);
    std::cerr << "FileWriter created\n";
    disk::SSTableBuilder builder{file_writer, kPageSize};
    std::cerr << "SSTableBuilder created\n";
    co_await builder.Init();
    std::cerr << "SSTableBuilder initialized\n";

    for (const auto& [key, value] : impl_) {
      co_await builder.Add(key, value);
    }
    co_await std::move(builder).Finish();
    std::cerr << "SSTableBuilder finished\n";
  }

  auto file_reader = io_manager.CreateFileReader(file_path);
  SSTable ss_table{std::move(file_reader)};
  co_await ss_table.Init();
  co_return ss_table;
}

}
