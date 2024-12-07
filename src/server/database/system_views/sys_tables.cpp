#include "sys_tables.hpp"

namespace structuredb::server::database::system_views {

SysTables::SysTables(Catalog catalog)
  : catalog_{std::move(catalog)}
{}

Awaitable<std::optional<std::string>> SysTables::Lookup(const std::string& key) {
  co_return co_await catalog_.GetStorageId(key);
}

}
