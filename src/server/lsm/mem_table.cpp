#include "mem_table.hpp"

#include <utils/find.hpp>

#include "disk/file_builder.hpp"

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

boost::asio::awaitable<SSTable> MemTable::Flush(boost::asio::io_context& io_context, const std::string& file_path) const {
  constexpr static const int64_t kPageSize = 8;

  disk::FileBuilder file_builder{file_path, kPageSize};
  for (const auto& [key, value] : impl_) {
    file_builder.Add(key, value);
  }
  std::move(file_builder).Finish();

  auto file = co_await disk::File::Load(io_context, file_path);
  co_return SSTable{std::move(file)};
}

}
