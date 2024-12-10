#pragma once

#include <map>

#include <lsm/lsm.hpp>

namespace structuredb::server::lsm {

class LsmRangeIterator : public Iterator {
public:
  static Awaitable<LsmRangeIterator> Create(Lsm& lsm, ScanRange range);

  bool HasMore() const override;

  Awaitable<Record> Next() override;
  explicit LsmRangeIterator() = delete;
private:
  explicit LsmRangeIterator(ScanRange range);

  const ScanRange range_;
  std::map<Record, Iterator::Ptr> queue_{};

  Awaitable<void> Add(Iterator::Ptr iter);
};

}

