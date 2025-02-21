#pragma once

#include "context.hpp"
#include "session.hpp"

namespace structuredb::server::database {

/// @brief main database class
class Database {
public:
 explicit Database(io::Manager& io_manager, std::string base_dir);

 Awaitable<void> Init();

 /// @returns storage by its id for recovery
 table::storage::Storage::Ptr GetStorageForRecover(const table::storage::Storage::Id& storage_id);

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
