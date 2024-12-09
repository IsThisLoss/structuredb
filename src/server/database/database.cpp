#include "database.hpp"

#include <spdlog/spdlog.h>

#include <wal/recovery.hpp>

#include "exceptions.hpp"

namespace structuredb::server::database {

namespace {

const std::string kSysTransactions = "sys_transactions";
const std::string kSysTables = "sys_tables";

}

Database::Database(io::Manager& io_manager, std::string base_dir)
    : context_{
      .io_manager = io_manager,
      .base_dir = base_dir,
    }
{}

Awaitable<void> Database::Init() {
  if (is_initialized_) {
    co_return;
  }

  // init sys tables
  // 1. sys_transactions
  {
    const auto path = context_.base_dir + "/" + kSysTransactions;
    co_await context_.io_manager.CreateDirectory(path);
    auto tx_table = std::make_shared<table::LsmStorage>(context_.io_manager, path, kSysTransactions);
    co_await tx_table->Init();
    context_.storages.try_emplace(kSysTransactions, tx_table);
    context_.tx_storage = std::make_shared<transaction::Storage>(std::move(tx_table));
  }

  // 2. sys_tables
  {
    const auto path = context_.base_dir + "/" + kSysTables;
    co_await context_.io_manager.CreateDirectory(path);
    auto sys_tables = std::make_shared<table::LsmStorage>(context_.io_manager, path, kSysTables);
    co_await sys_tables->Init();
    context_.storages.try_emplace(kSysTables, std::move(sys_tables));
  }

  // 3. search for tables
  auto dir_content = co_await context_.io_manager.ListDirectory(context_.base_dir);
  const auto& internal_tables = Catalog::GetInternalTableNames();
  std::erase_if(dir_content, [&internal_tables](const auto& name) { return internal_tables.contains(name); });

  // 4. init user tables
  for (const auto& name : dir_content) {
    SPDLOG_INFO("Going to init table {}", name);
    auto table = std::make_shared<table::LsmStorage>(context_.io_manager, context_.base_dir + "/" + name, name);
    co_await table->Init();
    context_.storages.try_emplace(name, std::move(table));
  }

  // recovery
  const auto wal_path = context_.base_dir + "/wal.sdb";
  const auto control_path = context_.base_dir + "/control.sdb";
  co_await wal::Recover(context_.io_manager, wal_path, control_path, *this);

  // start wal
  context_.wal_writer = co_await wal::Open(context_.io_manager, wal_path, control_path);
  for (const auto& [name, table] : context_.storages) {
    table->StartLogInto(context_.wal_writer);
    SPDLOG_INFO("Table {} is ready", name);
  }

  is_initialized_ = true;
}

table::LsmStorage::Ptr Database::GetStorageForRecover(const table::LsmStorage::Id& storage_id) {
  return context_.storages.at(storage_id);
}

Awaitable<Session> Database::StartSession(const std::optional<transaction::TransactionId>& tx) {
  if (!is_initialized_) {
    throw DatabaseException{"Database is not ready"};
  }
  Session session{context_};
  co_await session.Start(tx);
  co_return session;
}

}
