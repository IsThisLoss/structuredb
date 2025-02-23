#include "merge_iterator.hpp"

#include <spdlog/spdlog.h>

namespace structuredb::server::lsm {

Awaitable<MergeIterator> MergeIterator::Create(std::vector<Iterator::Ptr> iterators) {
  MergeIterator result{};
  for (auto& iterator : iterators) {
    co_await result.Add(std::move(iterator));
  }
  co_return result;
}

bool MergeIterator::HasMore() const {
  return !heap_.empty();
}

Awaitable<Record> MergeIterator::Next() {
  assert(HasMore());
  auto top = std::move(heap_.top());
  heap_.pop();

  co_await Add(std::move(top.iterator));
  co_return top.record;
}

Awaitable<void> MergeIterator::Add(Iterator::Ptr iter) {
  if (iter->HasMore()) {
    heap_.push(Item{
        .record = co_await iter->Next(),
        .iterator = std::move(iter)
    });
  } 
}

bool MergeIterator::Item::operator<(const MergeIterator::Item& other) const {
  return other.record < record;
}

}
