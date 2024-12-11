#pragma once

#include <lsm/lsm.hpp>

namespace structuredb::server::lsm {

/// @brief iterator over sorted range of keys
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

  struct Item {
    Record record{};
    Iterator::Ptr iterator{};

    bool operator<(const Item& other) const;
  };

  using MinHeap = std::priority_queue<Item>;

  MinHeap heap_{};

  Awaitable<void> Add(Iterator::Ptr iter);
};

}

