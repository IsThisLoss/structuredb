#pragma once

#include <sdb/buffer_reader.hpp>
#include <sdb/buffer_writer.hpp>

namespace structuredb::server::lsm::disk {
 
/// @brief page header
///
/// Always places at the begining of page
struct PageHeader {
  /// @property count of elements
  int64_t count;

  /// @property checksum of page content
  int64_t checksum;

  static Awaitable<PageHeader> Load(sdb::BufferReader& reader);

  static Awaitable<void> Flush(sdb::BufferWriter& writer, const PageHeader& header);

  static int64_t EstimateSize(const PageHeader& header);
};

}
