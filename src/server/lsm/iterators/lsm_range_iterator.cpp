#include "lsm_range_iterator.hpp"

namespace structuredb::server::lsm {

LsmRangeIterator::LsmRangeIterator(ScanRange range) 
  : range_{std::move(range)}
{}

Awaitable<LsmRangeIterator> LsmRangeIterator::Create(Lsm& lsm, ScanRange range) {
  LsmRangeIterator result{std::move(range)};

  std::vector<Iterator::Ptr> iterators;
  iterators.reserve(lsm.ro_mem_tables_.size() + lsm.ss_tables_.size() + 1);

  iterators.push_back(lsm.mem_table_.Scan(result.range_));
  for (auto& ro_mem_table : lsm.ro_mem_tables_) {
    iterators.push_back(ro_mem_table.Scan(result.range_));
  }
  for (auto& ss_table : lsm.ss_tables_) {
    iterators.push_back(co_await ss_table.Scan(result.range_));
  }

  result.impl_ = co_await MergeIterator::Create(std::move(iterators));

  co_return result;
}

bool LsmRangeIterator::HasMore() const {
  return impl_.HasMore();
}

Awaitable<Record> LsmRangeIterator::Next() {
  assert(HasMore());
  co_return co_await impl_.Next();
}

}
