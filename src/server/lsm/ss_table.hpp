#pragma once

#include <string>

#include <io/file_reader.hpp>

#include "disk/page.hpp"
#include "disk/ss_table_header.hpp"
#include "record_consumer.hpp"
#include "iterator.hpp"

namespace structuredb::server::lsm {

class SSTable {
public:
  static Awaitable<SSTable> Create(io::FileReader::Ptr file_reader);

  Awaitable<bool> Scan(const std::string& key, const RecordConsumer& consumer);

  Sequence GetMaxSeqNo() const;
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
  io::FileReader::Ptr file_reader_{};
  disk::SSTableHeader header_{};
  int64_t header_size_{};
  disk::Page page_{};

  std::unordered_map<size_t, disk::Page> page_cache_;

  Awaitable<disk::Page> GetPage(int64_t page_num);

  friend class SSTableIterator;
};

class SSTableIterator : public Iterator {
public:
  explicit SSTableIterator(SSTable& ss_table);

  bool HasMore() const override;

  Awaitable<Record> Next() override;

private:
  SSTable& ss_table_;
  int64_t current_page_{0};
  int64_t current_record_{0};
};

}
