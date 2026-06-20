#include "database.hpp"

#include <memory>
#include <string>
#include <optional>
#include <vector>
#include <spdlog/spdlog.h>

#include <table/storage/lsm_storage.hpp>
#include <utils/find.hpp>
#include <wal/recovery.hpp>

#include "exceptions.hpp"

namespace structuredb::server::database {

namespace {

const std::string kSysTransactions = "sys_transactions";
const std::string kSysTables = "sys_tables";

}

Database::Database(io::Manager& io_manager, std::string base_dir, lsm::Options lsm_options)
    : context_{
      .io_manager = io_manager,
      .base_dir = base_dir,
      .lsm_options = lsm_options,
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
    context_.tx_storage = std::make_shared<transaction::Storage>(context_.io_manager.Context());
  }

  // 2. sys_tables
  {
    const auto path = context_.base_dir + "/" + kSysTables;
    co_await context_.io_manager.CreateDirectory(path);
    auto sys_storage = std::make_shared<table::storage::LsmEngine>(context_.io_manager, path, kSysTables, context_.lsm_options);
    co_await sys_storage->Init();
    context_.storages.try_emplace(kSysTables, std::move(sys_storage));
  }

  // 3. search for tables
  auto dir_content = co_await context_.io_manager.ListDirectory(context_.base_dir);
  const auto& internal_tables = Catalog::GetInternalTableNames();
  std::erase_if(dir_content, [&internal_tables](const auto& name) { return internal_tables.contains(name); });

  // 4. init user tables
  for (const auto& name : dir_content) {
    SPDLOG_INFO("Going to init table {}", name);
    auto storage = std::make_shared<table::storage::LsmEngine>(context_.io_manager, context_.base_dir + "/" + name, name, context_.lsm_options);
    co_await storage->Init();
    context_.storages.try_emplace(name, std::move(storage));
  }

  // recovery
  const auto wal_path = context_.base_dir + "/wal";
  co_await wal::Recover(context_.io_manager, wal_path, *this);

  // start wal
  context_.wal_writer = co_await wal::Writer::Open(context_.io_manager, wal_path);
  context_.tx_storage->StartLogInto(context_.wal_writer);
  for (const auto& [name, table] : context_.storages) {
    if (auto durable = std::dynamic_pointer_cast<table::storage::DurableStorage>(table)) {
      durable->StartLogInto(context_.wal_writer);
    }
    SPDLOG_INFO("Table {} is ready", name);
  }

  is_initialized_ = true;
}

table::storage::DurableStorage::Ptr Database::GetStorageForRecover(const table::storage::StorageEngine::Id& storage_id) {
  return std::dynamic_pointer_cast<table::storage::DurableStorage>(context_.storages.at(storage_id));
}

Awaitable<table::storage::DurableStorage::Ptr> Database::EnsureStorageForRecover(
    const table::storage::StorageEngine::Id& storage_id
) {
  if (auto* existing = utils::FindOrNullptr(context_.storages, storage_id)) {
    co_return std::dynamic_pointer_cast<table::storage::DurableStorage>(*existing);
  }

  SPDLOG_INFO("Materializing replicated storage {}", storage_id);
  const auto path = context_.base_dir + "/" + storage_id;
  co_await context_.io_manager.CreateDirectory(path);
  auto storage = std::make_shared<table::storage::LsmEngine>(context_.io_manager, path, storage_id, context_.lsm_options);
  co_await storage->Init();
  if (context_.wal_writer) {
    storage->StartLogInto(context_.wal_writer);
  }
  const auto [it, _] = context_.storages.try_emplace(storage_id, std::move(storage));
  co_return std::dynamic_pointer_cast<table::storage::DurableStorage>(it->second);
}

transaction::Storage::Ptr Database::GetTransactionStorage() {
  return context_.tx_storage;
}

Awaitable<void> Database::Flush() {
  // snapshot the storages first: the loop below co_awaits, and a concurrent
  // CreateTable could rehash context_.storages and invalidate iterators
  std::vector<table::storage::StorageEngine::Ptr> storages;
  storages.reserve(context_.storages.size());
  for (const auto& [name, storage] : context_.storages) {
    storages.push_back(storage);
  }

  for (const auto& storage : storages) {
    co_await storage->Flush();
  }
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
