#pragma once

#include <sdb/reader.hpp>
#include <sdb/writer.hpp>

namespace structuredb::server::lsm::disk {

/// @brief sstable header
///
/// Always places at the begining of sstable
struct SSTableHeader {
  int64_t page_size{0};
  int64_t page_count{0};

  static Awaitable<SSTableHeader> Load(sdb::Reader& reader);

  static Awaitable<void> Flush(sdb::Writer& writer, const SSTableHeader& header);

  static size_t EstimateSize(const SSTableHeader& header);
};

}
