#include "page.hpp"

#include <spdlog/spdlog.h>

#include "page_header.hpp"
#include <lsm/exceptions.hpp>

#include <utils/crc.hpp>

#include <boost/algorithm/string/join.hpp>

namespace structuredb::server::lsm::disk {

Awaitable<Page> Page::Load(sdb::BufferReader& reader) {
  Page result{};
  const auto header = co_await PageHeader::Load(reader);
  utils::Crc crc{};

  SPDLOG_INFO("Load page: count = {}", header.count);

  result.keys_.reserve(header.count);
  result.seq_nos_.reserve(header.count);
  result.values_.reserve(header.count);
  for (int i = 0; i < header.count; i++) {
    result.keys_.push_back(co_await reader.ReadString());
    crc.Update(result.keys_.back());
    result.seq_nos_.push_back(co_await reader.ReadInt());
    crc.Update(result.seq_nos_.back());
    result.values_.push_back(co_await reader.ReadString());
    crc.Update(result.values_.back());
  }
  const auto checksum = static_cast<int64_t>(crc.Result());
  if (checksum != header.checksum) {
    auto msg = fmt::format("Failed to load page, checksums do not match {} != {}", checksum, header.checksum);
    throw CurraptedSSTable{std::move(msg)};
  }
  SPDLOG_DEBUG("Checksum match {}", checksum);
  SPDLOG_DEBUG("Loaded page keys: {}", boost::algorithm::join(result.keys_, ", "));
  co_return result;
}

int64_t Page::Find(const std::string& key) const {
  std::vector<std::string> result;
  auto it = std::ranges::lower_bound(keys_, key);
  const auto offset = std::distance(keys_.begin(), it);
  return offset;
}

Record Page::At(int64_t pos) {
  return Record{
    .key = keys_[pos],
    .seq_no = seq_nos_[pos],
    .value = values_[pos],
  };
}

const std::string& Page::MinKey() const {
  return keys_.front();
}

const std::string& Page::MaxKey() const {
  return keys_.back();
}

int64_t Page::Size() const {
  return static_cast<int64_t>(keys_.size());
}

}
