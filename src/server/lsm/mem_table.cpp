#include "mem_table.hpp"

#include <utils/find.hpp>

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

}
