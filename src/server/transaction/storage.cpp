#include "storage.hpp"
#include "transaction/types.hpp"

#include <io/types.hpp>
#include <utils/find.hpp>
#include <wal/events/tx_storage_commit_event.hpp>

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

Awaitable<void> Storage::AcquireRowLock(const TransactionId& tx, const std::string& row_key) {
  std::unique_lock lock(lock_mu_);

  // Wait until lock is available
  // This is condition variable + predicate check, not busy-wait
  // Other threads can proceed while we're blocked
  lock_cv_.wait(lock, [this, &row_key] {
    const auto* lock_state = utils::FindOrNullptr(row_locks_, row_key);
    return !lock_state || lock_state->holder == 0;
  });

  // Lock is now free, acquire it
  auto& lock_state = row_locks_[row_key];
  lock_state.holder = tx;
  tx_locks_[tx].insert(row_key);
  SPDLOG_DEBUG("Tx {} acquired lock on row {}", ToString(tx), row_key);
  co_return;
}

void Storage::ReleaseAllLocks(const TransactionId& tx) {
  const auto* locked_rows = utils::FindOrNullptr(tx_locks_, tx);
  if (!locked_rows) {
    return;
  }

  std::lock_guard lock(lock_mu_);
  size_t released_count = 0;

  for (const auto& row_key : *locked_rows) {
    auto* lock_state = utils::FindOrNullptr(row_locks_, row_key);
    if (lock_state && lock_state->holder == tx) {
      lock_state->holder = 0;  // Release lock

      // Grant lock to next waiter if any
      if (!lock_state->waiters.empty()) {
        auto next_tx = lock_state->waiters.front();
        lock_state->waiters.erase(lock_state->waiters.begin());
        lock_state->holder = next_tx;
        SPDLOG_DEBUG("Tx {} acquired lock on row {} (from waiter queue)", ToString(next_tx), row_key);
      }
      released_count++;
    }
  }

  tx_locks_.erase(tx);
  SPDLOG_DEBUG("Tx {} released {} locks", ToString(tx), released_count);

  // Notify all waiting coroutines (they will check condition and retry)
  lock_cv_.notify_all();
}

}
