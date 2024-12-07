#include "transactional_table.hpp"

#include <spdlog/spdlog.h>

namespace structuredb::server::table {

namespace {

struct TransactionalValue {
  transaction::TransactionId tx{};
  bool is_deleted{};
  std::string value{};
};

std::string ToString(const TransactionalValue& value) {
  std::string result;
  result.append(reinterpret_cast<const char*>(&value.tx.data), value.tx.size());
  result.append(reinterpret_cast<const char*>(&value.is_deleted), sizeof(bool));
  result.append(value.value);
  return result;
}

TransactionalValue ParseTransactionalValue(const std::string& data) {
  TransactionalValue result{};
  const char* ptr = data.data();
  ::memcpy(result.tx.data, ptr, result.tx.size());
  ptr += result.tx.size();
  result.is_deleted = *reinterpret_cast<const bool*>(ptr);
  ptr += sizeof(bool);
  result.value.assign(ptr, data.size() - result.tx.size());
  return result;
}

}

TransactionalTable::TransactionalTable(LsmStorage::Ptr lsm_storage, transaction::Storage::Ptr tx_storage, transaction::TransactionId tx)
  : lsm_storage_{std::move(lsm_storage)}, tx_storage_{std::move(tx_storage)}, tx_{std::move(tx)}
{}

Awaitable<void> TransactionalTable::Upsert(
      const std::string& key,
      const std::string& value
) {
  const auto transactional_value = ToString(TransactionalValue{
    .tx = tx_,
    .value = value,
  });
  co_await lsm_storage_->Upsert(key, transactional_value);
}

Awaitable<std::optional<std::string>> TransactionalTable::Lookup(const std::string& key) {
  SPDLOG_DEBUG("Lookup: tx = {}, key = {}", transaction::ToString(tx_), key);
  std::vector<TransactionalValue> candidates{};
  co_await lsm_storage_->Scan(key, [&](const auto& data) {
      candidates.push_back(ParseTransactionalValue(data));
      return false;
  });

  for (auto& candidate : candidates) {
    SPDLOG_DEBUG("Lookup candidate: tx = {}, value = {}", transaction::ToString(candidate.tx), candidate.value);
    if (candidate.tx == tx_ || co_await tx_storage_->IsCommited(candidate.tx)) {
      SPDLOG_DEBUG("Lookup candidate: tx = {}, value = {} will be returned", transaction::ToString(candidate.tx), candidate.value);
      if (candidate.is_deleted) {
        co_return std::nullopt;
      }
      co_return std::make_optional(std::move(candidate.value));
    }
    SPDLOG_DEBUG("Lookup next");
  }

  SPDLOG_DEBUG("Lookup not found: tx = {}, key = {}", transaction::ToString(tx_), key);
  co_return std::nullopt;
}

Awaitable<bool> TransactionalTable::Delete(const std::string& key) {
  const auto value = co_await Lookup(key);
  if (!value.has_value()) {
    co_return false;
  }
  const auto transactional_value = ToString(TransactionalValue{
    .tx = tx_,
    .is_deleted = true,
  });
  co_await lsm_storage_->Upsert(key, transactional_value);
  co_return true;
}

}
