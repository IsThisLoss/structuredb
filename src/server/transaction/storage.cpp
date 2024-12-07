#include "storage.hpp"

#include <spdlog/spdlog.h>

namespace structuredb::server::transaction {

namespace {

const std::string kStarted = "started";
const std::string kCommited = "commited";
const std::string kRollbacked = "rollbacked";

}

Storage::Storage(table::LsmStorage::Ptr lsm_storage)
  : lsm_storage_{std::move(lsm_storage)}
{}

Awaitable<TransactionId> Storage::Begin() {
  auto tx = transaction::GenerateTransactionId();
  co_await lsm_storage_->Upsert(ToBinary(tx), kStarted);
  co_return tx;
}

Awaitable<void> Storage::Rollback(const TransactionId& tx) {
  SPDLOG_DEBUG("Rollback transaction {}", ToString(tx));
  co_await lsm_storage_->Upsert(ToBinary(tx), kRollbacked);
}

Awaitable<void> Storage::Commit(const TransactionId& tx) {
  SPDLOG_DEBUG("Commit transaction {}", ToString(tx));
  co_await lsm_storage_->Upsert(ToBinary(tx), kCommited);
}

Awaitable<bool> Storage::IsCommited(const TransactionId& tx) {
  const auto tx_value = co_await lsm_storage_->Get(ToBinary(tx));
  co_return tx_value.has_value() && tx_value.value() == kCommited;
}

Awaitable<bool> Storage::IsStarted(const TransactionId& tx) {
  const auto tx_value = co_await lsm_storage_->Get(ToBinary(tx));
  co_return tx_value.has_value() && tx_value.value() == kStarted;
}

Awaitable<std::optional<std::string>> Storage::GetStatus(const TransactionId& tx) {
  co_return co_await lsm_storage_->Get(ToBinary(tx));
}

}
