#pragma once

#include <sdb/buffer_reader.hpp>
#include <sdb/buffer_writer.hpp>

#include <lsm/types.hpp>

namespace structuredb::server::lsm::disk {

/// @brief sstable header
///
/// Always places at the begining of sstable
struct SSTableHeader {
  int64_t page_size{0};
  int64_t page_count{0};
  Sequence max_seq_no{0};

  static Awaitable<SSTableHeader> Load(sdb::BufferReader& reader);

  static Awaitable<void> Flush(sdb::BufferWriter& writer, const SSTableHeader& header);

  static int64_t EstimateSize(const SSTableHeader& header);
};

}
