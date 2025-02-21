#include "session.hpp"

#include "catalog.hpp"
#include "exceptions.hpp"

#include "system_views/sys_tables.hpp"
#include "system_views/sys_transactions.hpp"

#include <table/storage/lsm_storage.hpp>

namespace structuredb::server::database {

namespace {

const std::string kSysTables = "sys_tables";
const std::string kSysTransactions = "sys_transactions";

}

Session::Session(
    Context& context
) : context_{context}
  {}

Awaitable<void> Session::Start(const std::optional<transaction::TransactionId>& tx) {
  if (tx.has_value()) {
    if (!co_await context_.tx_storage->IsStarted(tx.value())) {
      throw DatabaseException{"Invalid transaction id: " + transaction::ToString(tx.value())};
    }
    tx_ = tx.value();
    is_autocommit_ = false;
    co_return;
  }
  tx_ = co_await context_.tx_storage->Begin();
}

Awaitable<transaction::TransactionId> Session::Finish() {
  if (is_autocommit_) {
    co_await Commit();
  }
  co_return tx_;
}

transaction::TransactionId Session::GetTx() {
  return tx_;
}

Awaitable<void> Session::CreateTable(const std::string& name) {
  try {
    SPDLOG_INFO("Going to create table: {}", name);
    auto catalog = GetCatalog();
    const auto storage_id = co_await catalog.AddStorage(name);
    const auto path = context_.base_dir + "/" + storage_id;
    co_await context_.io_manager.CreateDirectory(path);
    auto storage = std::make_shared<table::storage::LsmStorage>(context_.io_manager, path, storage_id);
    co_await storage->Init();
    storage->StartLogInto(context_.wal_writer);
    context_.storages.try_emplace(storage_id, std::move(storage));
    SPDLOG_INFO("Table {} is created, id {}", name, storage_id);
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Failed to create table: {}", e.what());
    throw;
  }
}

Awaitable<void> Session::DropTable(const std::string& name) {
  try {
    auto catalog = GetCatalog();
    co_await catalog.DeleteStorage(name);
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Failed to drop table: {}", name);
  }
}

Awaitable<Session> Session::Begin() {
  // TODO check nested transactions
  Session result{context_};
  co_await result.Start();
  co_return result;
}

Awaitable<void> Session::Commit() {
  co_await context_.tx_storage->Commit(tx_);
}

Awaitable<table::Table::Ptr> Session::GetTable(const std::string& name) {
  auto catalog = GetCatalog();

  if (name == kSysTables) {
    co_return std::make_shared<system_views::SysTables>(std::move(catalog));
  }

  if (name == kSysTransactions) {
    co_return std::make_shared<system_views::SysTransactions>(context_.tx_storage);
  }

  const auto storage_id = co_await catalog.GetStorageId(name);
  SPDLOG_DEBUG("GetTable {}, {}", name, storage_id.value_or("<null>"));
  if (!storage_id.has_value()) {
    co_return nullptr;
  }
  const auto table = std::make_shared<table::TransactionalTable>(context_.storages.at(storage_id.value()), context_.tx_storage, tx_);
  co_return table;
}

Catalog Session::GetCatalog() const {
  return Catalog{
    std::make_shared<table::TransactionalTable>(
      context_.storages.at(kSysTables),
      context_.tx_storage,
      tx_
    ),
  };
}

Awaitable<void> Session::CompactTable(const std::string& name) {
  auto table = co_await GetTable(name);
  if (!table) {
    co_return;
  }
  co_await table->Compact();
}

}
