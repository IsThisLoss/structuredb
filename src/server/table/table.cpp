#include "table.hpp"

#include <iostream>

#include <wal/events/insert_event.hpp>

#include <database/database.hpp>

namespace structuredb::server::table {

namespace {

constexpr const int64_t kMaxTx = std::numeric_limits<int64_t>::max();

struct VersionedValue {
  /// @property number of transaction that created value
  int64_t created_tx{0};

  /// @property number of transaction that deleted value
  int64_t deleted_tx{kMaxTx};

  std::string value;
};

std::string ToString(const VersionedValue& value) {
  std::string result;
  result.append(reinterpret_cast<const char*>(&value.created_tx), sizeof(int64_t));
  result.append(reinterpret_cast<const char*>(&value.deleted_tx), sizeof(int64_t));
  result.append(value.value);
  return result;
}

VersionedValue ParseVersionedValue(const std::string& data) {
  VersionedValue result{};
  const char* ptr = data.data();
  result.created_tx = *reinterpret_cast<const int64_t*>(ptr);
  ptr += sizeof(int64_t);
  result.deleted_tx = *reinterpret_cast<const int64_t*>(ptr);
  ptr += sizeof(int64_t);
  result.value.assign(ptr, data.size() - 2*sizeof(int64_t));
  return result;
}

}

Table::Table(io::Manager& io_manager, const std::string& base_dir, database::Database& db)
  : logged_table_{io_manager, base_dir}, db_{db}
{}

void Table::StartLogInto(wal::Writer::Ptr wal_writer) {
  logged_table_.StartLogInto(std::move(wal_writer));
  std::cerr << "Start wal\n";
}

Awaitable<void> Table::RecoverRecord(
      const std::string& key,
      const lsm::Sequence seq_no,
      const std::string& value
) {
  co_await logged_table_.Upsert(key, value, seq_no);
}

Awaitable<void> Table::Upsert(
      const int64_t tx,
      const std::string& key,
      const std::string& value
) {
  const auto versioned_value = ToString(VersionedValue{
    .value = value,
    .created_tx = tx,
  });
  co_await logged_table_.Upsert(key, value);
}

Awaitable<std::optional<std::string>> Table::Lookup(const int64_t tx, const std::string& key) {
  std::cerr << "Lookup with tx: " << tx << std::endl;
  VersionedValue result{};
  co_await logged_table_.Lookup(key, [this, &tx, &result](const auto& data) {
      const auto value = ParseVersionedValue(data);
      auto& tx_storage = db_.GetTransactionStorage();
      if (tx_storage.IsCommited(value.deleted_tx) || value.deleted_tx == tx) {
        return false;
      }
      if (!tx_storage.IsCommited(value.created_tx) && value.created_tx != tx) {
        return false;
      }
      if (value.created_tx < result.created_tx) {
        return true;
      }
      result = value;
      return false;
  });
  if (result.created_tx == 0) {
    co_return std::nullopt;
  }
  co_return std::make_optional(std::move(result.value));
}

}
