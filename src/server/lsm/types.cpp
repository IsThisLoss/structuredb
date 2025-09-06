#include "types.hpp"

namespace structuredb::server::lsm {

bool Record::operator<(const Record& rhs) const {
  if (key != rhs.key) {
    return key < rhs.key;
  }
  if (seq_no != rhs.seq_no) {
    return seq_no > rhs.seq_no;
  }
  return value < rhs.value;
}

void Write(sdb::Writer& writer, const Record& record) {
  writer.WriteString(record.key);
  writer.WriteInt(record.seq_no);
  writer.WriteString(record.value);
}

void Read(sdb::Reader& reader, Record& record) {
  record.key = reader.ReadString();
  record.seq_no = reader.ReadInt();
  record.value = reader.ReadString();
}

ScanRange ScanRange::FullScan() {
  return ScanRange{};
}

}

