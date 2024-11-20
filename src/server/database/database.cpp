#include "database.hpp"

#include <iostream>
#include <unordered_set>

#include <wal/recovery.hpp>

#include "exceptions.hpp"

namespace structuredb::server::database {

namespace {

const std::string kSysTransactions = "sys_transactions";
const std::string kSysTables = "sys_tables";
const std::string kCreated = "created";
const std::string kDropped = "dropped";

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
        std::cerr << "Failed to initialize database: " << e.what() << std::endl;
        exit(1);
      }
      std::cerr << "DB initialized!\n";
  });
}

Awaitable<void> Database::Init() {
  // init sys tables
  // 1. sys_transactions
  {
    const auto path = base_dir_ + "/" + kSysTransactions;
    co_await io_manager_.CreateDirectory(path);
    tx_table_ = std::make_shared<table::LoggedTable>(io_manager_, path, kSysTransactions);
    co_await tx_table_->Init();
    tx_storage_ = std::make_shared<transaction::Storage>(tx_table_);
  }

  // 2. sys_tables
  {
    const auto path = base_dir_ + "/" + kSysTables;
    co_await io_manager_.CreateDirectory(path);
    sys_tables_ = std::make_shared<table::Table>(std::make_shared<table::LoggedTable>(io_manager_, path, kSysTables), tx_storage_);
    co_await sys_tables_->Init();
  }

  // 3. search for tables
  auto dir_content = co_await io_manager_.ListDirectory(base_dir_);
  std::erase_if(dir_content, [](const auto& name) { return kInternalTables.contains(name); });

  // 4. init user tables
  for (const auto& name : dir_content) {
    std::cerr << "Found table: " << name << std::endl;
    auto table = std::make_shared<table::Table>(std::make_shared<table::LoggedTable>(io_manager_, base_dir_ + "/" + name, name), tx_storage_);
    co_await table->Init();
    tables_.try_emplace(name, std::move(table));
  }

  // recovery
  const auto wal_path = base_dir_ + "/wal.sdb";
  const auto control_path = base_dir_ + "/control.sdb";
  co_await wal::Recover(io_manager_, wal_path, control_path, *this);

  // start wal
  wal_writer_ = co_await wal::Open(io_manager_, wal_path, control_path);
  tx_table_->StartLogInto(wal_writer_);
  sys_tables_->StartLogInto(wal_writer_);
  for (const auto& [name, table] : tables_) {
    table->StartLogInto(wal_writer_);
    std::cerr << "Table " << name << " ready\n";
  }
}

transaction::Storage::Ptr Database::GetTransactionStorage() {
  return tx_storage_;
}

Awaitable<void> Database::CreateTable(const transaction::TransactionId& tx, const std::string& name) {
  if (kInternalTables.contains(name) || tables_.contains(name)) {
    throw DatabaseException{"Table already exists"};
  }
  std::cerr << "Going to create table: " << name << std::endl;
  const auto path = base_dir_ + "/" + name;
  auto table = std::make_shared<table::Table>(std::make_shared<table::LoggedTable>(io_manager_, path, name), tx_storage_);
  co_await table->Init();
  tables_.try_emplace(name, std::move(table));
  co_await sys_tables_->Upsert(tx, name, kCreated);
  std::cerr << "Table " << name << " created" << std::endl;
}

Awaitable<void> Database::DropTable(const transaction::TransactionId& tx, const std::string& name) {
  if (kInternalTables.contains(name)) {
    throw DatabaseException{"Cannot drop system table"};
  }

  const auto table_status = co_await sys_tables_->Lookup(tx, name);
  if (!table_status.has_value() || table_status.value() != kCreated) {
    throw DatabaseException{"No such table"};
  }

  co_await sys_tables_->Upsert(tx, name, kDropped);
}

table::Table::Ptr Database::GetTableForRecover(const std::string& table_name) {
  return tables_.at(table_name);
}

Awaitable<table::Table::Ptr> Database::GetTable(const transaction::TransactionId& tx, const std::string& table_name) {
  const auto table_status = co_await sys_tables_->Lookup(tx, table_name);
  if (!table_status.has_value() || table_status.value() != kCreated) {
    co_return nullptr;
  }
  co_return tables_.at(table_name);
}

table::LoggedTable::Ptr Database::GetTxTable() {
  return tx_table_;
}

}
