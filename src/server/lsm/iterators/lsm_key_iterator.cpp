#include "lsm_key_iterator.hpp"

namespace structuredb::server::lsm {

Awaitable<LsmKeyIterator> LsmKeyIterator::Create(Lsm* lsm, std::string key) {
  LsmKeyIterator result{lsm, std::move(key)};
  result.current_iterator_ = result.lsm_->mem_table_.Scan(result.range_);
  co_await result.Advance();
  co_return result;
}

LsmKeyIterator::LsmKeyIterator(Lsm* lsm, std::string key) 
  : lsm_{lsm},
    range_{.lower_bound = key, .upper_bound = key},
    ro_mem_table_idx_{static_cast<int64_t>(lsm_->ro_mem_tables_.size()) - 1},
    ss_table_idx_{static_cast<int64_t>(lsm_->ss_tables_.size()) - 1}
{
}

bool LsmKeyIterator::HasMore() const {
  return (current_iterator_ && current_iterator_->HasMore());
}

Awaitable<Record> LsmKeyIterator::Next() {
  assert(HasMore());
  auto result = co_await current_iterator_->Next();
  co_await Advance();
  co_return result;
}

Awaitable<Iterator::Ptr> LsmKeyIterator::GetNextIterator() {
  if (ro_mem_table_idx_ >= 0) {
    co_return lsm_->ro_mem_tables_[ro_mem_table_idx_--].Scan(range_);
  }
  if (ss_table_idx_ >= 0) {
    co_return co_await lsm_->ss_tables_[ss_table_idx_--].Scan(range_);
  }
  co_return nullptr;
}

Awaitable<void> LsmKeyIterator::Advance() {
  while (current_iterator_ && !current_iterator_->HasMore()) {
    current_iterator_ = co_await GetNextIterator();
  }
}

}

