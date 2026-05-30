#pragma once

#include "types.hpp"
#include "wal/writer.hpp"
#include <table/table.hpp>
#include <unordered_set>

namespace structuredb::server::transaction {

/// @brief stores and provides transaction ids
class Storage {
public:
  using Ptr = std::shared_ptr<Storage>;

  explicit Storage() = default;

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

  /// @brief acquire exclusive lock on row for transaction
  ///
  /// Blocks until lock is acquired (currently synchronous with spin-wait)
  /// Lock is held until transaction commits or rolls back
  void AcquireRowLock(const TransactionId& tx, const std::string& row_key);

  /// @brief release all locks held by transaction
  void ReleaseAllLocks(const TransactionId& tx);

  void StartLogInto(wal::Writer::Ptr wal_writer);

  /// @brief recovers transaction status from wal
  Awaitable<void> RecoverFromLog(const TransactionId last_committed_tx_id);

private:
  constexpr static const TransactionId kInitialTxId = 10;
  TransactionId next_tx_id_{kInitialTxId};
  std::unordered_map<TransactionId, std::string> tx_status_;
  wal::Writer::Ptr wal_writer_;

  // Row locking (row_key -> tx_id holding exclusive lock, 0 if free)
  std::unordered_map<std::string, TransactionId> row_locks_;
  // Track which rows each tx has locked (for cleanup on commit/rollback)
  std::unordered_map<TransactionId, std::unordered_set<std::string>> tx_locks_;
};

}
