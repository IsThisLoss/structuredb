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

}
