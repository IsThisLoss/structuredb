#pragma once

#include <cstdint>
#include <sdb/reader.hpp>
#include <sdb/writer.hpp>

namespace structuredb::server::lsm::disk {
 
/// @brief page header
///
/// Always places at the begining of page
struct PageHeader {
  /// @property count of elements
  int64_t count;

  /// @property checksum of page content
  int64_t checksum;

  static int64_t SdbSize();
};

void Write(sdb::Writer& writer, const PageHeader& header);

void Read(sdb::Reader& reader, PageHeader& header);

}
