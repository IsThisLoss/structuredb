#include "ss_table_header.hpp"

namespace structuredb::server::lsm::disk {

Awaitable<SSTableHeader> SSTableHeader::Load(sdb::Reader& reader) {
  SSTableHeader result{};
  result.page_size = co_await reader.ReadInt();
  result.page_count = co_await reader.ReadInt();
  co_return result;
}

Awaitable<void> SSTableHeader::Flush(sdb::Writer& writer, const SSTableHeader& header) {
  co_await writer.WriteInt(header.page_size);
  co_await writer.WriteInt(header.page_count);
}

size_t SSTableHeader::EstimateSize(const SSTableHeader& header) {
  return sdb::Writer::EstimateSize(header.page_size) + sdb::Writer::EstimateSize(header.page_count);
}

}
