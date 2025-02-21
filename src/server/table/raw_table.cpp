#include "raw_table.hpp"

namespace structuredb::server::table {

RawTable::RawTable(storage::Storage::Ptr table_storage)
  : table_storage_{std::move(table_storage)}
{}

Awaitable<void> RawTable::Upsert(
    const std::string& key,
    const std::string& value
) {
  co_await table_storage_->Upsert(Row{key, value});
}

Awaitable<std::optional<std::string>> RawTable::Lookup(const std::string& key) {
  auto iter = co_await table_storage_->Scan(key);
  if (!iter->HasMore()) {
    co_return std::nullopt;
  }
  auto row = co_await iter->Next();
  co_return std::move(row.value);
}

Awaitable<bool> RawTable::Delete(const std::string& key) {
  throw std::runtime_error{"delete operation is not supported by raw table"};
}

Awaitable<Iterator::Ptr> RawTable::Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound) {
  co_return co_await table_storage_->Scan(lower_bound, upper_bound);
}

Awaitable<void> RawTable::Compact() {
  co_return;
}

}
