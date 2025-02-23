#include "table.hpp"

namespace structuredb::server::table::sync {

Table::Table(
  io::Manager& io_manager,
  table::Table::Ptr impl
) : io_manager_{io_manager}
 , impl_{std::move(impl)}
{
  assert(impl_);
}

void Table::Upsert(
    const std::string& key,
    const std::string& value
) {
  io_manager_.RunSync(impl_->Upsert(key, value));
}

std::optional<std::string> Table::Lookup(const std::string& key) {
  return io_manager_.RunSync(impl_->Lookup(key));
}

bool Table::Delete(const std::string& key) {
  return io_manager_.RunSync(impl_->Delete(key));
}

sync::Iterator::Ptr Table::Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound) {
  auto async_iterator = io_manager_.RunSync(impl_->Scan(lower_bound, upper_bound));
  if (async_iterator == nullptr) {
    return nullptr;
  }
  return std::make_shared<sync::Iterator>(io_manager_, std::move(async_iterator));
}

void Table::Compact() {
  io_manager_.RunSync(impl_->Compact());
}

}
