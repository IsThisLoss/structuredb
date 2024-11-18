#pragma once

#include <set>

#include "types.hpp"

namespace structuredb::server::transaction {

/// @brief stores and provides transaction ids 
class Storage {
public:
  /// @brief starts transaction
  TransactionId Begin();

  /// @brief rollback transaction
  void Rollback(TransactionId tx);
 
  /// @brief commits transaction
  void Commit(TransactionId tx);

  bool IsCommited(TransactionId tx);

  void SetMinCommitedTx(TransactionId tx);

  TransactionId GetPersistedTx() const;
private:
  TransactionId sequence_{1};

  TransactionId persisted_tx_{};

  TransactionId min_commited_tx_{};

  std::set<TransactionId> commited_{};

  void Compact();
};

}
