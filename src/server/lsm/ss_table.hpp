#pragma once

#include <string>

#include <io/file_reader.hpp>
#include "disk/ss_table_header.hpp"
#include "disk/page.hpp"

namespace structuredb::server::lsm {

class SSTable {
public:
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
  disk::SSTableHeader header_{};
  disk::Page page_{};

  Awaitable<void> SetFilePos(size_t pos);
};

}
