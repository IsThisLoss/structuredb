#include "page_header.hpp"

#include <iostream>

namespace structuredb::server::lsm::disk {

Awaitable<PageHeader> PageHeader::Load(sdb::Reader& reader) {
  std::cerr << "Start loading page header" << std::endl;
  PageHeader result{};
  result.count = co_await reader.ReadInt();
  std::cerr << "Page header count: " << result.count << std::endl;
  co_return result;
}

Awaitable<void> PageHeader::Flush(sdb::Writer& writer, const PageHeader& header) {
  co_await writer.WriteInt(header.count);
}

size_t PageHeader::EstimateSize(const PageHeader& header) {
  return sdb::Writer::EstimateSize(header.count);
}

}
