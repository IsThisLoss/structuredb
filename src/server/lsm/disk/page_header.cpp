#include "page_header.hpp"

#include <iostream>

namespace structuredb::server::lsm::disk {

Awaitable<PageHeader> PageHeader::Load(sdb::BufferReader& reader) {
  PageHeader result{};
  result.count = co_await reader.ReadInt();
  co_return result;
}

Awaitable<void> PageHeader::Flush(sdb::BufferWriter& writer, const PageHeader& header) {
  co_await writer.WriteInt(header.count);
}

size_t PageHeader::EstimateSize(const PageHeader& header) {
  return sdb::BufferWriter::EstimateSize(header.count);
}

}
