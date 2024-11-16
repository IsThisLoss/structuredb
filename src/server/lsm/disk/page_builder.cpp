#include "page_builder.hpp"

#include <iostream>

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
  values_.clear();
  current_size_ = 0;
}

bool PageBuilder::IsEnoughPlace(const std::string& key, const std::string& value) const {
  const size_t next_record_size = sdb::Writer::EstimateSize(key) + sdb::Writer::EstimateSize(value);
  return current_size_ + next_record_size < max_bytes_size_;
}

void PageBuilder::Add(const std::string& key, const std::string& value) {
  const size_t next_record_size = sdb::Writer::EstimateSize(key) + sdb::Writer::EstimateSize(value);
  keys_.push_back(key);
  values_.push_back(value);
  current_size_ += next_record_size;
}

bool PageBuilder::IsEmpty() const {
  return keys_.empty();
}

Awaitable<void> PageBuilder::Flush(io::FileWriter& writer) {
  PageHeader header{
    .count = static_cast<int64_t>(keys_.size()),
  };
  std::cerr << "Flush page builder: " << header.count << std::endl;
  sdb::BufferWriter buffer_writer{max_bytes_size_};
  co_await PageHeader::Flush(buffer_writer, header);
  for (size_t i = 0; i < header.count; i++) {
    co_await buffer_writer.WriteString(keys_[i]);
    co_await buffer_writer.WriteString(values_[i]);
  }

  auto raw = std::move(buffer_writer).Extract();
  co_await writer.Write(raw.data(), raw.size());
}

}
