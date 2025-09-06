#pragma once

#include <vector>

#include <lsm/disk/page_header.hpp>

#include <utils/crc.hpp>

namespace structuredb::server::lsm::disk {

class PageBuilder {
public:
  explicit PageBuilder(int64_t page_size);

  bool IsEnoughSpace(size_t record_size) const;

  void AddRecord(const std::vector<char>& raw);

  std::vector<char> Extract() &&;

  void Clear();

  bool Empty() const;
private:
  int64_t page_size_;
  std::vector<char> current_page_;
  PageHeader header_;

  void FlushHeader();
};

}
