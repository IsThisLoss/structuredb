#include "ss_table_iterator.hpp"

namespace structuredb::server::lsm {

SSTableIterator::SSTableIterator(SSTable& ss_table, ScanRange range)
  : ss_table_{ss_table}, range_{std::move(range)}
{}

Awaitable<SSTableIterator> SSTableIterator::Create(SSTable& ss_table, ScanRange range) {
  SSTableIterator result{ss_table, std::move(range)};
  if (result.range_.lower_bound.has_value()) {
    result.current_page_ = co_await result.ss_table_.LowerBound(result.range_.lower_bound.value());
    if (result.current_page_ < result.ss_table_.header_.page_count) {
      auto start_page = co_await result.ss_table_.GetPage(result.current_page_);
      result.current_record_ = start_page.Find(result.range_.lower_bound.value());
    }
  }
  if (result.HasMore()) {
    result.next_ = co_await result.NextImpl();
  }
  co_return result;
}

bool SSTableIterator::HasMore() const {
  if (current_page_ >= ss_table_.header_.page_count) {
    return false;
  }
  
  if (range_.upper_bound.has_value()) {
    return next_.key <= range_.upper_bound.value();
  }
  return true;
}

Awaitable<Record> SSTableIterator::Next() {
  auto result = std::move(next_);
  next_ = co_await NextImpl();
  co_return result;
}

Awaitable<Record> SSTableIterator::NextImpl() {
  assert(current_page_ < ss_table_.header_.page_count);

  auto page = co_await ss_table_.GetPage(current_page_);
  auto result = page.At(current_record_);
  current_record_++;
  if (current_record_ >= page.Size()) {
    current_record_ = 0;
    current_page_++;
  }
  co_return result;
}

}
