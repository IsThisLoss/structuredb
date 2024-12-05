#include "table.hpp"

#include <spdlog/spdlog.h>

namespace structuredb::server::table {

namespace {

struct TransactionalValue {
  transaction::TransactionId tx{};

  std::string value{};
};

std::string ToString(const TransactionalValue& value) {
  std::string result;
  result.append(reinterpret_cast<const char*>(&value.tx.data), value.tx.size());
  result.append(value.value);
  return result;
}

TransactionalValue ParseTransactionalValue(const std::string& data) {
  TransactionalValue result{};
  const char* ptr = data.data();
  ::memcpy(result.tx.data, ptr, result.tx.size());
  ptr += result.tx.size();
  result.value.assign(ptr, data.size() - result.tx.size());
  return result;
}

}

Table::Table(LoggedTable::Ptr logged_table, transaction::Storage::Ptr tx_storage, transaction::TransactionId tx)
  : logged_table_{std::move(logged_table)}, tx_storage_{std::move(tx_storage)}, tx_{std::move(tx)}
{}

Awaitable<void> Table::Upsert(
      const std::string& key,
      const std::string& value
) {
  const auto transactional_value = ToString(TransactionalValue{
    .tx = tx_,
    .value = value,
  });
  co_await logged_table_->Upsert(key, transactional_value);
}

Awaitable<std::optional<std::string>> Table::Lookup(const std::string& key) {
  SPDLOG_DEBUG("Lookup: tx = {}, key = {}", transaction::ToString(tx_), key);
  std::vector<TransactionalValue> candidates{};
  co_await logged_table_->Scan(key, [&](const auto& data) {
      candidates.push_back(ParseTransactionalValue(data));
      return false;
  });

  for (auto& candidate : candidates) {
    SPDLOG_DEBUG("Lookup candidate: tx = {}, value = {}", transaction::ToString(candidate.tx), candidate.value);
    if (candidate.tx == tx_ || co_await tx_storage_->IsCommited(candidate.tx)) {
      SPDLOG_DEBUG("Lookup candidate: tx = {}, value = {} will be returned", transaction::ToString(candidate.tx), candidate.value);
      co_return std::make_optional(std::move(candidate.value));
    }
    SPDLOG_DEBUG("Lookup next");
  }

  SPDLOG_DEBUG("Lookup not found: tx = {}, key = {}", transaction::ToString(tx_), key);
  co_return std::nullopt;
}

}
