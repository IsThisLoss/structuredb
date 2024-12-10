#pragma once

#include <lsm/mem_table.hpp>

#include "iterator.hpp"

namespace structuredb::server::lsm {

class MemTableIterator : public Iterator {
public:
  explicit MemTableIterator(const MemTable& mem_table, const ScanRange& range);

  bool HasMore() const override;

  Awaitable<Record> Next() override;
private:
  const MemTable& mem_table_;
  const ScanRange range_;
  std::multiset<Record>::iterator it_;
};

}

