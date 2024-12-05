#include "catalog.hpp"

#include <spdlog/spdlog.h>

#include <utils/uuid.hpp>
#include "exceptions.hpp"

namespace structuredb::server::database {

std::string ToString(const Catalog::TableInfo& info) {
  std::string result;
  result.append(reinterpret_cast<const char*>(&info.status), sizeof(int64_t));
  result.append(info.id);
  return result;
}

Catalog::TableInfo ParseRecord(const std::string& data) {
  Catalog::TableInfo result{};
  const char* ptr = data.data();
  ::memcpy(&result.status, ptr, sizeof(int64_t));
  ptr += sizeof(int64_t);
  result.id.assign(ptr, data.size() - sizeof(int64_t));
  return result;
}

Catalog::Catalog(
    table::Table::Ptr table
) : table_{std::move(table)}
{}

Awaitable<std::string> Catalog::AddTable(const std::string& name) {
  // check exists
  {
    const auto table_info = co_await GetTableInfo(name);
    if (table_info.has_value() && table_info.value().status == Catalog::TableStatus::kCreated) {
      throw DatabaseException{"Table " + name + " already exists"};
    }
  }

  const TableInfo new_table_info{
    .status = TableStatus::kCreated,
    .id = utils::ToString(utils::GenerateUuid()),
  };
  co_await table_->Upsert(name, ToString(new_table_info));
  co_return new_table_info.id;
}

Awaitable<void> Catalog::DeleteTable(const std::string& name) {
  auto table_info = co_await GetTableInfo(name);
  if (!table_info.has_value() || table_info.value().status == Catalog::TableStatus::kDropped) {
    co_return;
  }
  table_info.value().status = Catalog::TableStatus::kDropped;
  co_await table_->Upsert(name, ToString(table_info.value()));
}

Awaitable<std::optional<std::string>> Catalog::GetTableId(const std::string& name) {
  auto table_info = co_await GetTableInfo(name);
  if (!table_info.has_value() || table_info.value().status == Catalog::TableStatus::kDropped) {
    co_return std::nullopt;
  }
  co_return table_info.value().id;
}

Awaitable<std::optional<Catalog::TableInfo>> Catalog::GetTableInfo(const std::string& name) {
  const auto raw = co_await table_->Lookup(name);
  if (!raw.has_value()) {
    co_return std::nullopt;
  }
  const auto record = ParseRecord(raw.value());
  co_return record;
}

}
