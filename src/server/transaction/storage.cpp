#include "storage.hpp"

#include <iostream>

namespace structuredb::server::transaction {
 
TransactionId Storage::Begin() {
  return ++sequence_;
}

void Storage::Rollback(TransactionId) {
  // does nothing for now
}

void Storage::Commit(TransactionId tx) {
  commited_.insert(tx);
  Compact();
}

bool Storage::IsCommited(TransactionId tx) {
  std::cerr << "IsCommited: " << tx << " " << min_commited_tx_ << std::endl;
  return tx <= min_commited_tx_ || commited_.contains(tx);
}

void Storage::SetMinCommitedTx(TransactionId tx) {
  min_commited_tx_ = tx;
  sequence_ = min_commited_tx_;
}

TransactionId Storage::GetPersistedTx() const {
  return persisted_tx_;
}

void Storage::Compact() {
  for (auto it = commited_.begin(); it != commited_.end();) {
    if (*it != min_commited_tx_ + 1) {
      break;
    }
    min_commited_tx_ = *it;
    it = commited_.erase(it);
  }
}

}
