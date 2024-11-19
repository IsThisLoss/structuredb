#include "storage.hpp"

#include <iostream>

namespace structuredb::server::transaction {

namespace {

std::string kFirstTx = "1";
std::string kTxCounterKey = "counter";
std::string kStarted = "started";
std::string kCommited = "commited";
std::string kRollbacked = "rollbacked";

}

Storage::Storage(table::LoggedTable::Ptr logged_table)
  : logged_table_{std::move(logged_table)}
{}

Awaitable<TransactionId> Storage::Begin() {
  auto counter_value = co_await logged_table_->Get(kTxCounterKey);
  const auto tx_key = counter_value.value_or(kFirstTx);
  co_await logged_table_->Upsert(tx_key, kStarted);
  auto result = std::stoll(tx_key);
  co_await logged_table_->Upsert(kTxCounterKey, std::to_string(result+1));
  co_return result;
}

Awaitable<void> Storage::Rollback(const TransactionId tx) {
  const auto tx_key = std::to_string(tx);
  co_await logged_table_->Upsert(tx_key, kRollbacked);
}

Awaitable<void> Storage::Commit(const TransactionId tx) {
  const auto tx_key = std::to_string(tx);
  co_await logged_table_->Upsert(tx_key, kCommited);
}

Awaitable<bool> Storage::IsCommited(TransactionId tx) {
  std::cerr << "IsCommited\n";
  const auto tx_key = std::to_string(tx);
  const auto tx_value = co_await logged_table_->Get(tx_key);
  co_return tx_value.has_value() && tx_value.value() == kCommited;
}

Awaitable<bool> Storage::IsStarted(const TransactionId tx) {
  const auto tx_key = std::to_string(tx);
  const auto tx_value = co_await logged_table_->Get(tx_key);
  co_return tx_value.has_value() && tx_value.value() == kStarted;
}

}
