#include "storage.hpp"
#include "transaction/types.hpp"

#include <io/types.hpp>
#include <utils/find.hpp>
#include <wal/events/tx_storage_commit_event.hpp>

#include <thread>
#include <spdlog/spdlog.h>

namespace structuredb::server::transaction {

namespace {

const std::string kStarted = "started";
const std::string kCommited = "commited";
const std::string kRollbacked = "rollbacked";

}

Awaitable<TransactionId> Storage::Begin() {
  const auto tx = next_tx_id_++;
  tx_status_[tx] = kStarted;
  co_return tx;
}

Awaitable<void> Storage::Rollback(const TransactionId& tx) {
  SPDLOG_DEBUG("Rollback transaction {}", ToString(tx));
  tx_status_[tx] = kRollbacked;
  co_return;
}

Awaitable<void> Storage::Commit(const TransactionId& tx) {
  SPDLOG_DEBUG("Commit transaction {}", ToString(tx));
  tx_status_[tx] = kCommited;
  if (wal_writer_) {
    co_await wal_writer_->Write(std::make_unique<wal::TxStorageCommitEvent>(tx));
  }
  // Release all locks held by this transaction
  ReleaseAllLocks(tx);
}

Awaitable<bool> Storage::IsCommited(const TransactionId& tx) {
  const auto* tx_status = utils::FindOrNullptr(tx_status_, tx);
  co_return tx_status && *tx_status == kCommited;
}

Awaitable<bool> Storage::IsStarted(const TransactionId& tx) {
  const auto* tx_status = utils::FindOrNullptr(tx_status_, tx);
  co_return tx_status && *tx_status == kStarted;
}

Awaitable<std::optional<std::string>> Storage::GetStatus(const TransactionId& tx) {
  const auto* tx_status = utils::FindOrNullptr(tx_status_, tx);
  co_return tx_status ? std::optional<std::string>(*tx_status) : std::nullopt;
}

void Storage::StartLogInto(wal::Writer::Ptr wal_writer) {
  wal_writer_ = std::move(wal_writer);
}

Awaitable<void> Storage::RecoverFromLog(const TransactionId last_committed_tx_id) {
  tx_status_[last_committed_tx_id] = kCommited;
  next_tx_id_ = std::max(next_tx_id_, last_committed_tx_id + 1);
  co_return;
}

void Storage::AcquireRowLock(const TransactionId& tx, const std::string& row_key) {
  // Simple spin-wait loop until lock is available
  // TODO: Use proper async/await mechanism for better performance
  while (true) {
    auto it = row_locks_.find(row_key);
    if (it == row_locks_.end() || it->second == 0) {
      // Lock is free, acquire it
      row_locks_[row_key] = tx;
      tx_locks_[tx].insert(row_key);
      SPDLOG_DEBUG("Tx {} acquired lock on row {}", ToString(tx), row_key);
      return;
    }

    // Lock is held by another tx, yield and retry
    // In a production system, use condition variable or async notification
    std::this_thread::yield();
  }
}

void Storage::ReleaseAllLocks(const TransactionId& tx) {
  auto it = tx_locks_.find(tx);
  if (it == tx_locks_.end()) {
    return;
  }

  const auto& locked_rows = it->second;
  for (const auto& row_key : locked_rows) {
    auto row_it = row_locks_.find(row_key);
    if (row_it != row_locks_.end() && row_it->second == tx) {
      row_it->second = 0;  // Release lock
    }
  }

  tx_locks_.erase(it);
  SPDLOG_DEBUG("Tx {} released {} locks", ToString(tx), locked_rows.size());
}

}
