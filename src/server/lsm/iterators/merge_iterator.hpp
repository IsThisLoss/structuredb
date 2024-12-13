#pragma once

#include "iterator.hpp"

namespace structuredb::server::lsm {

/// @brief created new iterator from list of other iterators
///
/// New iterator provides sorted output across all input iterators
class MergeIterator : public Iterator {
public:
  static Awaitable<MergeIterator> Create(std::vector<Iterator::Ptr> iterators);

  bool HasMore() const override;

  Awaitable<Record> Next() override;
private:
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
