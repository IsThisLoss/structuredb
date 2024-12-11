#pragma once

#include "types.hpp"
#include <table/lsm_storage.hpp>

namespace structuredb::server::transaction {

/// @brief stores and provides transaction ids 
class Storage {
public:
  using Ptr = std::shared_ptr<Storage>;

  explicit Storage(table::LsmStorage::Ptr lsm_storage);

  /// @brief starts transaction
  Awaitable<TransactionId> Begin();

  /// @brief rollback transaction
  Awaitable<void> Rollback(const TransactionId& tx);
 
  /// @brief commits transaction
  Awaitable<void> Commit(const TransactionId& tx);

  /// @brief returns true if tx commited
  Awaitable<bool> IsCommited(const TransactionId& tx);

  /// @brief returns true if tx started
  Awaitable<bool> IsStarted(const TransactionId& tx);

  /// @brief returns status of transaction by its id
  Awaitable<std::optional<std::string>> GetStatus(const TransactionId& tx);
private:
  table::LsmStorage::Ptr lsm_storage_;
};

}
