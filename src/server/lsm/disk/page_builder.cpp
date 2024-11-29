#include "page_builder.hpp"

#include <spdlog/spdlog.h>

#include <boost/crc.hpp>

#include "page_header.hpp"

namespace structuredb::server::lsm::disk {

PageBuilder::PageBuilder(const int64_t max_bytes_size)
  : max_bytes_size_{max_bytes_size}
{
  Clear();
}

void PageBuilder::Clear() {
  current_size_ = PageHeader::EstimateSize(PageHeader{});
  keys_.clear();
  seq_nos_.clear();
  values_.clear();
  crc_.Clear();
}

bool PageBuilder::IsEnoughPlace(const Record& record) const {
  const size_t next_record_size = sdb::Writer::EstimateSize(record.key) + sdb::Writer::EstimateSize(record.seq_no) + sdb::Writer::EstimateSize(record.value);
  return current_size_ + next_record_size < max_bytes_size_;
}

void PageBuilder::Add(const Record& record) {
  const size_t next_record_size = sdb::Writer::EstimateSize(record.key) + sdb::Writer::EstimateSize(record.seq_no) + sdb::Writer::EstimateSize(record.value);

  keys_.push_back(record.key);
  crc_.Update(record.key);

  seq_nos_.push_back(record.seq_no);
  crc_.Update(record.seq_no);

  values_.push_back(record.value);
  crc_.Update(record.value);

  current_size_ += next_record_size;
}

bool PageBuilder::IsEmpty() const {
  return keys_.empty();
}

Awaitable<void> PageBuilder::Flush(io::FileWriter& writer) {
  PageHeader header{
    .count = static_cast<int64_t>(keys_.size()),
    .checksum = static_cast<int64_t>(crc_.Result()),
  };
  SPDLOG_INFO("Flush page builder: count = {}, size = {}/{}, checksum = {}", header.count, current_size_, max_bytes_size_, header.checksum);
  sdb::BufferWriter buffer_writer{max_bytes_size_};
  co_await PageHeader::Flush(buffer_writer, header);
  for (size_t i = 0; i < header.count; i++) {
    co_await buffer_writer.WriteString(keys_[i]);
    co_await buffer_writer.WriteInt(seq_nos_[i]);
    co_await buffer_writer.WriteString(values_[i]);
  }

  auto raw = std::move(buffer_writer).Extract();
  co_await writer.Write(raw.data(), raw.size());
}

}
