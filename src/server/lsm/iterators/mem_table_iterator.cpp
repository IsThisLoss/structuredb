#include "mem_table_iterator.hpp"

namespace structuredb::server::lsm {

MemTableIterator::MemTableIterator(const MemTable& mem_table, const ScanRange& range)
  : mem_table_{mem_table}, range_{range}
{
  if (range.lower_bound.has_value()) {
    auto start_record = Record{.key = range.lower_bound.value()};
    it_ = mem_table_.impl_.lower_bound(start_record);
  } else {
    it_ = mem_table_.impl_.begin();
  }
}

bool MemTableIterator::HasMore() const {
  if (it_ == mem_table_.impl_.end()) {
    return false;
  }
  if (range_.upper_bound.has_value()) {
    return it_->key <= range_.upper_bound.value();
  }
  return true;
}

Awaitable<Record> MemTableIterator::Next() {
  auto result = *it_;
  SPDLOG_INFO("Mem next: {}", result.value);
  it_ = std::next(it_);
  co_return result;
}

}

