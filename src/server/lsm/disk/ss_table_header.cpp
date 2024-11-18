#include "ss_table_header.hpp"

namespace structuredb::server::lsm::disk {

Awaitable<SSTableHeader> SSTableHeader::Load(sdb::BufferReader& reader) {
  SSTableHeader result{};
  result.page_size = co_await reader.ReadInt();
  result.page_count = co_await reader.ReadInt();
  result.max_seq_no = co_await reader.ReadInt();
  co_return result;
}

Awaitable<void> SSTableHeader::Flush(sdb::BufferWriter& writer, const SSTableHeader& header) {
  co_await writer.WriteInt(header.page_size);
  co_await writer.WriteInt(header.page_count);
  co_await writer.WriteInt(header.max_seq_no);
}

int64_t SSTableHeader::EstimateSize(const SSTableHeader& header) {
  return sdb::BufferWriter::EstimateSize(header.page_size)
    + sdb::BufferWriter::EstimateSize(header.page_count)
    + sdb::BufferWriter::EstimateSize(header.max_seq_no);
}

}
