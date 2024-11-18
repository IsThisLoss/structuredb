#include "page.hpp"

#include <iostream>

#include "page_header.hpp"

namespace structuredb::server::lsm::disk {

Awaitable<Page> Page::Load(sdb::BufferReader& reader) {
  Page result{};
  const auto header = co_await PageHeader::Load(reader);
  result.keys_.reserve(header.count);
  result.seq_nos_.reserve(header.count);
  result.values_.reserve(header.count);
  for (int i = 0; i < header.count; i++) {
    result.keys_.push_back(co_await reader.ReadString());
    result.seq_nos_.push_back(co_await reader.ReadInt());
    result.values_.push_back(co_await reader.ReadString());
  }
  co_return result;
}

std::optional<std::string> Page::Find(const std::string& key) const {
  auto it = std::lower_bound(keys_.begin(), keys_.end(), key);
  if (it == keys_.end() || *it != key) {
    return std::nullopt;
  }
  const auto offset = std::distance(keys_.begin(), it);
  auto result_it = std::next(values_.begin(), offset);
  return std::make_optional(*result_it);
}

const std::string& Page::MinKey() const {
  return keys_.front();
}

const std::string& Page::MaxKey() const {
  return keys_.back();
}

}
