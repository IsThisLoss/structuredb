#include "page_builder.hpp"

#include <cstdint>
#include <algorithm>
#include <vector>
#include <lsm/disk/page_checksum.hpp>
#include <sdb/buffer_writer.hpp>

#include <spdlog/spdlog.h>

namespace structuredb::server::lsm::disk {

PageBuilder::PageBuilder(int64_t page_size)
  : page_size_{page_size},
    header_{
      .count = 0,
      .checksum = 0,
    }
{
  Clear();
}

bool PageBuilder::IsEnoughSpace(size_t record_size) const {
  return current_page_.size() + record_size <= static_cast<size_t>(page_size_);
}

void PageBuilder::AddRecord(const std::vector<char>& raw) {
  current_page_.insert(current_page_.end(), raw.begin(), raw.end());
  header_.count += 1;
}

std::vector<char> PageBuilder::Extract() && {
  if (current_page_.size() < static_cast<size_t>(page_size_)) {
    current_page_.resize(page_size_, 0);
  }

  header_.checksum = GetPageChecksum(current_page_);

  FlushHeader();
  auto result = std::move(current_page_);
  current_page_.clear();
  return result;
}

void PageBuilder::Clear() {
  current_page_.clear();
  current_page_.reserve(page_size_);
  header_.count = 0;
  header_.checksum = 0;
  // reserve space for header
  current_page_.resize(PageHeader::SdbSize(), 0);
}

bool PageBuilder::Empty() const {
  return header_.count == 0;
}

void PageBuilder::FlushHeader() {
  sdb::BufferWriter writer;
  Write(writer, header_);
  const auto raw = std::move(writer).Extract();
  std::copy(raw.begin(), raw.end(), current_page_.begin());
}

}
