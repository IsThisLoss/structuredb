#pragma once

#include <lsm/lsm.hpp>

#include "merge_iterator.hpp"

namespace structuredb::server::lsm {

/// @brief iterator over sorted range of all lsm keys
class LsmRangeIterator : public Iterator {
public:
  /// @brief creates iterator
  ///
  /// Do no call it directly, use Lsm::Scan
  static Awaitable<LsmRangeIterator> Create(Lsm& lsm, ScanRange range);

  bool HasMore() const override;

  Awaitable<Record> Next() override;
private:
  explicit LsmRangeIterator(ScanRange range);

  const ScanRange range_;

  MergeIterator impl_;
};

}

