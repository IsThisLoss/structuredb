#include "page_header.hpp"

namespace structuredb::server::lsm::disk {

Awaitable<PageHeader> PageHeader::Load(sdb::BufferReader& reader) {
  PageHeader result{};
  result.count = co_await reader.ReadInt();
  result.checksum = co_await reader.ReadInt();
  co_return result;
}

Awaitable<void> PageHeader::Flush(sdb::BufferWriter& writer, const PageHeader& header) {
  co_await writer.WriteInt(header.count);
  co_await writer.WriteInt(header.checksum);
}

size_t PageHeader::EstimateSize(const PageHeader& header) {
  return sdb::BufferWriter::EstimateSize(header.count) + sdb::BufferWriter::EstimateSize(header.checksum);
}

}
