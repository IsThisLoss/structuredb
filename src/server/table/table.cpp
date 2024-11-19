#include "table.hpp"

#include <iostream>

#include <database/database.hpp>

namespace structuredb::server::table {

namespace {

struct TransactionalValue {
  transaction::TransactionId tx{0};

  std::string value{};
};

std::string ToString(const TransactionalValue& value) {
  std::string result;
  result.append(reinterpret_cast<const char*>(&value.tx), sizeof(int64_t));
  result.append(value.value);
  return result;
}

TransactionalValue ParseTransactionalValue(const std::string& data) {
  TransactionalValue result{};
  const char* ptr = data.data();
  result.tx = *reinterpret_cast<const int64_t*>(ptr);
  ptr += sizeof(int64_t);
  result.value.assign(ptr, data.size() - sizeof(int64_t));
  return result;
}

}

Table::Table(LoggedTable::Ptr logged_table, transaction::Storage::Ptr tx_storage)
  : logged_table_{std::move(logged_table)}, tx_storage_{std::move(tx_storage)}
{}

Awaitable<void> Table::Init() {
  co_await logged_table_->Init();
}

void Table::StartLogInto(wal::Writer::Ptr wal_writer) {
  logged_table_->StartLogInto(std::move(wal_writer));
}

Awaitable<void> Table::RecoverFromLog(
      const lsm::Sequence seq_no,
      const std::string& key,
      const std::string& value
) {
  co_await logged_table_->RecoverFromLog(seq_no, key, value);
}

Awaitable<void> Table::Upsert(
      const int64_t tx,
      const std::string& key,
      const std::string& value
) {
  const auto transactional_value = ToString(TransactionalValue{
    .tx = tx,
    .value = value,
  });
  co_await logged_table_->Upsert(key, transactional_value);
}

Awaitable<std::optional<std::string>> Table::Lookup(const int64_t tx, const std::string& key) {
  std::cerr << "Lookup with tx: " << tx << std::endl;
  std::vector<TransactionalValue> candidates{};
  co_await logged_table_->Scan(key, [&](const auto& data) {
      candidates.push_back(ParseTransactionalValue(data));
      return false;
  });

  for (auto& candidate : candidates) {
    std::cerr << "candidate: " << candidate.value << std::endl;
    if (candidate.tx == tx || co_await tx_storage_->IsCommited(candidate.tx)) {
      co_return std::make_optional(std::move(candidate.value));
    }
  }

  co_return std::nullopt;
}

}
