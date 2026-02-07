#pragma once

#include "types.hpp"
#include "wal/writer.hpp"
#include <table/table.hpp>

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

  void StartLogInto(wal::Writer::Ptr wal_writer);
  
  /// @brief recovers transaction status from wal
  Awaitable<void> RecoverFromLog(const TransactionId last_committed_tx_id);
private:
  constexpr static const TransactionId kInitialTxId = 10; 
  TransactionId next_tx_id_{kInitialTxId};
  std::unordered_map<TransactionId, std::string> tx_status_;
  wal::Writer::Ptr wal_writer_;
};

}
