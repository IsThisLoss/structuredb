#include "ss_table_header.hpp"

namespace structuredb::server::lsm::disk {

int64_t SSTableHeader::SdbSize() {
  return sizeof(int64_t) + sizeof(int64_t) + sizeof(int64_t);
}

void Write(sdb::Writer& writer, const SSTableHeader& header) {
  writer.WriteInt(header.page_size);
  writer.WriteInt(header.page_count);
  writer.WriteInt(header.max_seq_no);
}

void Read(sdb::Reader& reader, SSTableHeader& header) {
  header.page_size = reader.ReadInt();
  header.page_count = reader.ReadInt();
  header.max_seq_no = reader.ReadInt();
}

}
