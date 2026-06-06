#pragma once

#include <string>
#include <optional>

#include <lsm/options.hpp>
#include <table/storage/durable_storage.hpp>

#include "context.hpp"
#include "session.hpp"

namespace structuredb::server::database {

/// @brief main database class
class Database {
public:
 explicit Database(io::Manager& io_manager, std::string base_dir, lsm::Options lsm_options = {});

 Awaitable<void> Init();

 /// @returns storage by its id for recovery
 table::storage::DurableStorage::Ptr GetStorageForRecover(const table::storage::StorageEngine::Id& storage_id);

 transaction::Storage::Ptr GetTransactionStorage();

 /// @brief persists in-memory data of every table to disk
 ///
 /// driven by a background job; keeps the flush off the write path
 Awaitable<void> Flush();

  /// @brief starts session
  ///
  /// Session is required to perform any operation over database
  ///
  /// @tx - optional transaction id,
  /// if tx is provided session attaches to given transaction
  /// otherwise sesstion starts new transaction
  Awaitable<Session> StartSession(const std::optional<transaction::TransactionId>& tx = std::nullopt);

 ~Database() = default;
private:
 Context context_;
 bool is_initialized_{false};
};

}
