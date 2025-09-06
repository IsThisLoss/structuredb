#pragma once

#include <sdb/reader.hpp>
#include <sdb/writer.hpp>

#include <lsm/types.hpp>

namespace structuredb::server::lsm::disk {

/// @brief sstable header
///
/// Always places at the begining of sstable
struct SSTableHeader {
  int64_t page_size{0};
  int64_t page_count{0};
  Sequence max_seq_no{0};

  static int64_t SdbSize();
};

void Write(sdb::Writer& writer, const SSTableHeader& header);

void Read(sdb::Reader& reader, SSTableHeader& header);

}
