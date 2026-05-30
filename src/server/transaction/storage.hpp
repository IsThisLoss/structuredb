#pragma once

#include "types.hpp"
#include "wal/writer.hpp"
#include <optional>
#include <table/table.hpp>
#include <io/condition_variable.hpp>
#include <unordered_set>

namespace structuredb::server::transaction {

/// @brief stores and provides transaction ids
class Storage {
public:
  using Ptr = std::shared_ptr<Storage>;

  explicit Storage(boost::asio::io_context& io_context);

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
  /// Suspends (without blocking the event loop) until the row is free, then
  /// marks it as held. The lock is held until the transaction commits or rolls back.
  Awaitable<void> AcquireRowLock(const TransactionId& tx, const std::string& row_key);

  /// @brief release all locks held by transaction and wake waiting coroutines
  void ReleaseAllLocks(const TransactionId& tx);

  void StartLogInto(wal::Writer::Ptr wal_writer);

  /// @brief recovers transaction status from wal
  Awaitable<void> RecoverFromLog(const TransactionId last_committed_tx_id);

private:
  constexpr static const TransactionId kInitialTxId = 10;
  TransactionId next_tx_id_{kInitialTxId};
  std::unordered_map<TransactionId, std::string> tx_status_;
  wal::Writer::Ptr wal_writer_;

  // Coroutine-friendly notification for row-lock availability.
  // No std::mutex is needed: the io_context is single-threaded, so coroutines
  // run cooperatively and the maps below are only touched between co_await points.
  io::ConditionVariable lock_available_;

  // Row locking (row_key -> tx_id holding exclusive lock; absent means free)
  std::unordered_map<std::string, TransactionId> row_locks_;
  // Track which rows each tx has locked (for cleanup on commit/rollback)
  std::unordered_map<TransactionId, std::unordered_set<std::string>> tx_locks_;
};

}
