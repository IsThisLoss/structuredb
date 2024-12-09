#include "catalog.hpp"

#include <spdlog/spdlog.h>

#include <utils/uuid.hpp>

#include "exceptions.hpp"

namespace structuredb::server::database {

const static std::string kSysTransactions = "sys_transactions";
const static std::string kSysTables = "sys_tables";

const static std::unordered_set<std::string> kInternalTables{
  kSysTransactions,
  kSysTables,
  "wal.sdb",
  "control.sdb",
};

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
) : sys_tables_{std::move(table)}
{}

const std::unordered_set<std::string>& Catalog::GetInternalTableNames() {
  return kInternalTables;
}

Awaitable<std::string> Catalog::AddStorage(const std::string& name) {
  // check exists
  if (kInternalTables.contains(name)) {
    throw DatabaseException{"Table already exists"};
  }

  {
    const auto sys_tables_info = co_await GetTableInfo(name);
    if (sys_tables_info.has_value() && sys_tables_info.value().status == Catalog::TableStatus::kCreated) {
      throw DatabaseException{"Table " + name + " already exists"};
    }
  }

  const TableInfo new_sys_tables_info{
    .status = TableStatus::kCreated,
    .id = utils::ToString(utils::GenerateUuid()),
  };
  co_await sys_tables_->Upsert(name, ToString(new_sys_tables_info));
  co_return new_sys_tables_info.id;
}

Awaitable<void> Catalog::DeleteStorage(const std::string& name) {
  if (kInternalTables.contains(name)) {
    throw DatabaseException{"Cannot drop system table"};
  }

  auto table_info = co_await GetTableInfo(name);
  if (!table_info.has_value() || table_info.value().status == Catalog::TableStatus::kDropped) {
    co_return;
  }
  table_info.value().status = Catalog::TableStatus::kDropped;
  co_await sys_tables_->Upsert(name, ToString(table_info.value()));
}

Awaitable<std::optional<std::string>> Catalog::GetStorageId(const std::string& name) {
  auto sys_tables_info = co_await GetTableInfo(name);
  if (!sys_tables_info.has_value() || sys_tables_info.value().status == Catalog::TableStatus::kDropped) {
    co_return std::nullopt;
  }
  co_return sys_tables_info.value().id;
}

Awaitable<std::optional<Catalog::TableInfo>> Catalog::GetTableInfo(const std::string& name) {
  const auto raw = co_await sys_tables_->Lookup(name);
  if (!raw.has_value()) {
    co_return std::nullopt;
  }
  const auto record = ParseRecord(raw.value());
  SPDLOG_DEBUG("GetTableInfo: {} {}", record.id, static_cast<int>(record.status));
  co_return record;
}

}
