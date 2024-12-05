#include "storage.hpp"

#include <spdlog/spdlog.h>

namespace structuredb::server::transaction {

namespace {

std::string kStarted = "started";
std::string kCommited = "commited";
std::string kRollbacked = "rollbacked";

}

Storage::Storage(table::LoggedTable::Ptr logged_table)
  : logged_table_{std::move(logged_table)}
{}

Awaitable<TransactionId> Storage::Begin() {
  auto tx = transaction::GenerateTransactionId();
  co_await logged_table_->Upsert(ToBinary(tx), kStarted);
  SPDLOG_DEBUG("Begin transaction {}: {}", ToString(tx), ToBinary(tx));
  co_return tx;
}

Awaitable<void> Storage::Rollback(const TransactionId& tx) {
  SPDLOG_DEBUG("Rollback transaction {}", ToString(tx));
  co_await logged_table_->Upsert(ToBinary(tx), kRollbacked);
}

Awaitable<void> Storage::Commit(const TransactionId& tx) {
  SPDLOG_DEBUG("Commit transaction {}", ToString(tx));
  co_await logged_table_->Upsert(ToBinary(tx), kCommited);
}

Awaitable<bool> Storage::IsCommited(const TransactionId& tx) {
  const auto tx_value = co_await logged_table_->Get(ToBinary(tx));
  co_return tx_value.has_value() && tx_value.value() == kCommited;
}

Awaitable<bool> Storage::IsStarted(const TransactionId& tx) {
  const auto tx_value = co_await logged_table_->Get(ToBinary(tx));
  co_return tx_value.has_value() && tx_value.value() == kStarted;
}

}
