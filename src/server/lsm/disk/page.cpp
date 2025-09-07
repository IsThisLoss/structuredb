#include "page.hpp"

#include <spdlog/spdlog.h>
#include <boost/algorithm/string/join.hpp>

#include <lsm/disk/page_checksum.hpp>
#include <lsm/disk/page_header.hpp>
#include <lsm/exceptions.hpp>
#include <sdb/buffer_reader.hpp>
#include <utils/crc.hpp>

namespace structuredb::server::lsm::disk {

Page::Ptr Page::Load(std::vector<char>&& buffer) {
  const auto checksum = GetPageChecksum(buffer);

  sdb::BufferReader reader{std::move(buffer)};

  auto result = std::make_shared<Page>();
  PageHeader header{};
  Read(reader, header);

  if (checksum != header.checksum) {
    auto msg = fmt::format("Failed to load page, checksums do not match {} != {}", checksum, header.checksum);
    throw CurraptedSSTable{std::move(msg)};
  }
  SPDLOG_INFO("Load page: count = {}, checksum = {}", header.count, header.checksum);

  result->keys_.reserve(header.count);
  result->seq_nos_.reserve(header.count);
  result->values_.reserve(header.count);

  lsm::Record record{};
  for (int i = 0; i < header.count; i++) {
    Read(reader, record);
    result->keys_.push_back(record.key);
    result->seq_nos_.push_back(record.seq_no);
    result->values_.push_back(record.value);
  }
  return result;
}

int64_t Page::Find(const std::string& key) const {
  std::vector<std::string> result;
  auto it = std::ranges::lower_bound(keys_, key);
  const auto offset = std::distance(keys_.begin(), it);
  return offset;
}

Record Page::At(int64_t pos) const {
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
