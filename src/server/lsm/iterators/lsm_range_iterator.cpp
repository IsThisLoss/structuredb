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
  return !queue_.empty();
}

Awaitable<Record> LsmRangeIterator::Next() {
  assert(HasMore());
  auto it = queue_.begin();
  auto result = it->first;
  co_await Add(it->second);
  queue_.erase(it);
  co_return result;
}

Awaitable<void> LsmRangeIterator::Add(Iterator::Ptr iter) {
  if (iter->HasMore()) {
    queue_.try_emplace(co_await iter->Next(), iter);
  }
  co_return;
}

}
