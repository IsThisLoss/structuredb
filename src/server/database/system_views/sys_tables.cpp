#include "sys_tables.hpp"

namespace structuredb::server::database::system_views {

SysTables::SysTables(Catalog catalog)
  : catalog_{std::move(catalog)}
{}

Awaitable<std::optional<std::string>> SysTables::Lookup(const std::string& key) {
  co_return co_await catalog_.GetStorageId(key);
}

Awaitable<table::Iterator::Ptr> SysTables::Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound) {
  co_return co_await catalog_.Scan(lower_bound, upper_bound);
}

}
