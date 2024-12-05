#include "database.hpp"

#include <unordered_set>

#include <spdlog/spdlog.h>

#include <wal/recovery.hpp>

#include "exceptions.hpp"
#include "catalog.hpp"

namespace structuredb::server::database {

namespace {

const std::string kSysTransactions = "sys_transactions";
const std::string kSysTables = "sys_tables";

std::unordered_set<std::string> kInternalTables{
  kSysTransactions,
  kSysTables,
  "wal.sdb",
  "control.sdb",
};

}

Database::Database(io::Manager& io_manager, const std::string& base_dir)
  : io_manager_{io_manager}
  , base_dir_{base_dir}
{
  io_manager_.CoSpawn([this]() -> Awaitable<void> {
      try {
        co_await Init();
      } catch (const std::exception& e) {
        SPDLOG_ERROR("Failed to initialize database: {}", e.what());
        exit(1);
      }
      SPDLOG_INFO("Database is initialized");
      SPDLOG_DEBUG("Some debug");
  });
}

Awaitable<void> Database::Init() {
  // init sys tables
  // 1. sys_transactions
  {
    const auto path = base_dir_ + "/" + kSysTransactions;
    co_await io_manager_.CreateDirectory(path);
    auto tx_table = std::make_shared<table::LoggedTable>(io_manager_, path, kSysTransactions);
    co_await tx_table->Init();
    tables_.try_emplace(kSysTransactions, tx_table);
    tx_storage_ = std::make_shared<transaction::Storage>(std::move(tx_table));
  }

  // 2. sys_tables
  {
    const auto path = base_dir_ + "/" + kSysTables;
    co_await io_manager_.CreateDirectory(path);
    auto sys_tables = std::make_shared<table::LoggedTable>(io_manager_, path, kSysTables);
    co_await sys_tables->Init();
    tables_.try_emplace(kSysTables, std::move(sys_tables));
  }

  // 3. search for tables
  auto dir_content = co_await io_manager_.ListDirectory(base_dir_);
  std::erase_if(dir_content, [](const auto& name) { return kInternalTables.contains(name); });

  // 4. init user tables
  for (const auto& name : dir_content) {
    SPDLOG_INFO("Going to init table {}", name);
    auto table = std::make_shared<table::LoggedTable>(io_manager_, base_dir_ + "/" + name, name);
    co_await table->Init();
    tables_.try_emplace(name, std::move(table));
  }

  // recovery
  const auto wal_path = base_dir_ + "/wal.sdb";
  const auto control_path = base_dir_ + "/control.sdb";
  co_await wal::Recover(io_manager_, wal_path, control_path, *this);

  // start wal
  wal_writer_ = co_await wal::Open(io_manager_, wal_path, control_path);
  for (const auto& [name, table] : tables_) {
    table->StartLogInto(wal_writer_);
    SPDLOG_INFO("Table {} is ready", name);
  }
}

transaction::Storage::Ptr Database::GetTransactionStorage() {
  return tx_storage_;
}

Awaitable<void> Database::CreateTable(const transaction::TransactionId& tx, const std::string& name) {
  if (kInternalTables.contains(name)) {
    throw DatabaseException{"Table already exists"};
  }

  try {
    SPDLOG_INFO("Going to create table: {}", name);
    Catalog catalog(std::make_shared<table::Table>(tables_.at(kSysTables), tx_storage_, tx));
    const auto table_id = co_await catalog.AddTable(name);
    const auto path = base_dir_ + "/" + table_id;
    co_await io_manager_.CreateDirectory(path);
    auto table = std::make_shared<table::LoggedTable>(io_manager_, path, table_id);
    co_await table->Init();
    tables_.try_emplace(table_id, std::move(table));
    SPDLOG_INFO("Table {} is created, id {}", name, table_id);
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Failed to create table: {}", e.what());
    throw;
  }
}

Awaitable<void> Database::DropTable(const transaction::TransactionId& tx, const std::string& name) {
  if (kInternalTables.contains(name)) {
    throw DatabaseException{"Cannot drop system table"};
  }
  Catalog catalog(std::make_shared<table::Table>(tables_.at(kSysTables), tx_storage_, tx));
  co_await catalog.DeleteTable(name);
}

table::LoggedTable::Ptr Database::GetTableForRecover(const std::string& table_id) {
  return tables_.at(table_id);
}

Awaitable<table::Table::Ptr> Database::GetTable(const transaction::TransactionId& tx, const std::string& name) {
  Catalog catalog(std::make_shared<table::Table>(tables_.at(kSysTables), tx_storage_, tx));
  const auto table_id = co_await catalog.GetTableId(name);
  if (!table_id.has_value()) {
    co_return nullptr;
  }
  co_return std::make_shared<table::Table>(tables_.at(table_id.value()), tx_storage_, tx);
}

}
