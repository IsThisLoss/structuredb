#include "page_checksum.hpp"

#include "page_header.hpp"

#include <vector>
#include <utils/crc.hpp>

namespace structuredb::server::lsm::disk {

int64_t GetPageChecksum(const std::vector<char>& page_buffer) {
  utils::Crc crc{};
  crc.Update(page_buffer.data() + PageHeader::SdbSize(), page_buffer.size() - PageHeader::SdbSize());
  return static_cast<int64_t>(crc.Result());
}

}
