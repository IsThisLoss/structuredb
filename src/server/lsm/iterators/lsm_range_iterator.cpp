#include "lsm_range_iterator.hpp"

namespace structuredb::server::lsm {

LsmRangeIterator::LsmRangeIterator(ScanRange range) 
  : range_{std::move(range)}
{}

Awaitable<LsmRangeIterator> LsmRangeIterator::Create(Lsm& lsm, ScanRange range) {
  LsmRangeIterator result{std::move(range)};

  co_await result.Add(lsm.mem_table_.Scan(result.range_));

  for (auto& ro_mem_table : lsm.ro_mem_tables_) {
    co_await result.Add(ro_mem_table.Scan(result.range_));
  }

  for (auto& ss_table : lsm.ss_tables_) {
    co_await result.Add(co_await ss_table.Scan(result.range_));
  }

  co_return result;
}

bool LsmRangeIterator::HasMore() const {
  return !heap_.empty();
}

Awaitable<Record> LsmRangeIterator::Next() {
  assert(HasMore());
  auto top = std::move(heap_.top());
  heap_.pop();

  co_await Add(std::move(top.iterator));
  co_return top.record;
}

Awaitable<void> LsmRangeIterator::Add(Iterator::Ptr iter) {
  if (iter->HasMore()) {
    heap_.push(Item{
        .record = co_await iter->Next(),
        .iterator = std::move(iter)
    });
  }
  co_return;
}

bool LsmRangeIterator::Item::operator<(const LsmRangeIterator::Item& other) const {
  return other.record < record;
}

}
