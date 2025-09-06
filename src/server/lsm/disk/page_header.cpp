#include "page_header.hpp"

namespace structuredb::server::lsm::disk {

int64_t PageHeader::SdbSize() {
  return sizeof(int64_t) + sizeof(int64_t);
}

void Write(sdb::Writer& writer, const PageHeader& header) {
  writer.WriteInt(header.count);
  writer.WriteInt(header.checksum);
}

void Read(sdb::Reader& reader, PageHeader& header) {
  header.count = reader.ReadInt();
  header.checksum = reader.ReadInt();
}

}
