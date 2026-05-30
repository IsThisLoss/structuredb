#include "storage.hpp"
#include "transaction/types.hpp"

#include <optional>
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

Storage::Storage(boost::asio::io_context& io_context)
  : lock_available_{io_context}
{}

Awaitable<TransactionId> Storage::Begin() {
  const auto tx = next_tx_id_++;
  tx_status_[tx] = kStarted;
  co_return tx;
}

Awaitable<void> Storage::Rollback(const TransactionId& tx) {
  SPDLOG_DEBUG("Rollback transaction {}", ToString(tx));
  tx_status_[tx] = kRollbacked;
  // Release all locks held by this transaction
  ReleaseAllLocks(tx);
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
  // Suspend until the row is free. The predicate is re-checked each time a
  // transaction releases its locks. Single-threaded cooperative scheduling
  // guarantees no other coroutine runs between the predicate succeeding and us
  // recording the lock below, so the check-then-acquire is effectively atomic.
  co_await lock_available_.Wait([this, &row_key] {
    return !row_locks_.contains(row_key);
  });

  row_locks_[row_key] = tx;
  tx_locks_[tx].insert(row_key);
  SPDLOG_DEBUG("Tx {} acquired lock on row {}", ToString(tx), row_key);
}

void Storage::ReleaseAllLocks(const TransactionId& tx) {
  const auto* locked_rows = utils::FindOrNullptr(tx_locks_, tx);
  if (!locked_rows) {
    return;
  }

  for (const auto& row_key : *locked_rows) {
    const auto* holder = utils::FindOrNullptr(row_locks_, row_key);
    if (holder && *holder == tx) {
      row_locks_.erase(row_key);
    }
  }

  SPDLOG_DEBUG("Tx {} released {} locks", ToString(tx), locked_rows->size());
  tx_locks_.erase(tx);

  // Wake every waiting coroutine so each can re-check whether its row is free.
  lock_available_.NotifyAll();
}

}
