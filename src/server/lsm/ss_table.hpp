#pragma once

#include <string>

#include <io/file_reader.hpp>

#include "disk/ss_table_header.hpp"
#include "disk/page.hpp"

namespace structuredb::server::lsm {

class SSTable {
public:
  static Awaitable<SSTable> Create(io::FileReader&& file_reader);

  explicit SSTable(io::FileReader&& file_reader);

  Awaitable<void> Init();

  Awaitable<std::optional<std::string>> Get(const std::string& key);
private:
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
  io::FileReader file_reader_;
  sdb::Reader sdb_reader_;
  disk::SSTableHeader header_{};
  int64_t header_size_{};
  disk::Page page_{};

  Awaitable<void> SetFilePos(size_t pos);
};

}
