#pragma once

#include <sdb/reader.hpp>
#include <sdb/writer.hpp>

namespace structuredb::server::lsm::disk {
 
/// @brief page header
///
/// Always places at the begining of page
struct PageHeader {
  /// @property count of elements
  int64_t count;

  static Awaitable<PageHeader> Load(sdb::Reader& reader);

  static Awaitable<void> Flush(sdb::Writer& writer, const PageHeader& header);

  static size_t EstimateSize(const PageHeader& header);
};

}
