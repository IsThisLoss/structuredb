#pragma once

#include "types.hpp"
#include "wal/writer.hpp"
#include <table/table.hpp>
#include <unordered_set>
#include <mutex>
#include <condition_variable>

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
  /// Async lock acquisition - other coroutines can run while waiting
  /// Lock is held until transaction commits or rolls back
  Awaitable<void> AcquireRowLock(const TransactionId& tx, const std::string& row_key);

  /// @brief release all locks held by transaction
  /// @note Must be called from synchronous context (e.g., from Commit/Rollback)
  void ReleaseAllLocks(const TransactionId& tx);

  void StartLogInto(wal::Writer::Ptr wal_writer);

  /// @brief recovers transaction status from wal
  Awaitable<void> RecoverFromLog(const TransactionId last_committed_tx_id);

private:
  struct LockState {
    TransactionId holder{0};  // 0 means unlocked
    std::vector<TransactionId> waiters;  // Queue of waiting transactions
  };

  constexpr static const TransactionId kInitialTxId = 10;
  TransactionId next_tx_id_{kInitialTxId};
  std::unordered_map<TransactionId, std::string> tx_status_;
  wal::Writer::Ptr wal_writer_;

  // Synchronization for row-level locking
  mutable std::mutex lock_mu_;
  std::condition_variable lock_cv_;  // Notifies waiting coroutines

  // Row locking state (row_key -> lock info)
  std::unordered_map<std::string, LockState> row_locks_;
  // Track which rows each tx has locked (for cleanup on commit/rollback)
  std::unordered_map<TransactionId, std::unordered_set<std::string>> tx_locks_;
};

}
