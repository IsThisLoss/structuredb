#pragma once

#include <string>

#include <io/file_reader.hpp>

#include "disk/ss_table_header.hpp"
#include "disk/page.hpp"
#include "record_consumer.hpp"

namespace structuredb::server::lsm {

class SSTable {
public:
  static Awaitable<SSTable> Create(io::FileReader::Ptr file_reader);

  Awaitable<void> Get(const std::string& key, const RecordConsumer& consumer);
private:
  explicit SSTable(io::FileReader::Ptr file_reader);

  Awaitable<void> Init();

  /***** Structure of file ******/
  /* SSTableHeader:
   *   page_size: int64
   *   page_count: int64
   * Pages:
   *   Page:
   *     PageHeader:
   *       size:   int64
   *    keys:   string[]
   *    values: string[]
   */
  sdb::Reader sdb_reader_;
  disk::SSTableHeader header_{};
  int64_t header_size_{};
  disk::Page page_{};

  Awaitable<void> SetFilePos(size_t pos);
};

}
