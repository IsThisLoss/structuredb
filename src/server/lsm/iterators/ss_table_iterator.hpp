#pragma once

#include <lsm/ss_table.hpp>

#include "iterator.hpp"

namespace structuredb::server::lsm {

class SSTableIterator : public Iterator {
public:
  static Awaitable<SSTableIterator> Create(SSTable& ss_table, ScanRange range);

  bool HasMore() const override;

  Awaitable<Record> Next() override;
private:
  explicit SSTableIterator(SSTable& ss_table, ScanRange range);

  SSTable& ss_table_;
  const ScanRange range_;
  int64_t current_page_{0};
  int64_t current_record_{0};
  Record next_{};

  Awaitable<Record> NextImpl();
};

}

