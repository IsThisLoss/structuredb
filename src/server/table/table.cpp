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
  : lsm_{io_manager, base_dir}, db_{db}
{}

void Table::StartWal(wal::Writer::Ptr wal_writer) {
  wal_writer_ = std::move(wal_writer);
  std::cerr << "Start wal\n";
}

Awaitable<void> Table::Upsert(const int64_t tx, const std::string& key, const std::string& value) {
  const auto versioned_value = ToString(VersionedValue{
    .value = value,
    .created_tx = tx,
  });

  auto flushed_mem_table = co_await lsm_.Put(key, versioned_value);

  if (wal_writer_) {
    co_await wal_writer_->Write(std::make_unique<wal::InsertEvent>(tx, key, value));
    if (flushed_mem_table.has_value()) {
      flushed_mem_table.value().ScanValues([this](const auto& data) {
        const auto value = ParseVersionedValue(data);
        if (value.created_tx > max_tx_) {
          max_tx_ = value.created_tx;
        }
        if (value.deleted_tx > max_tx_) {
          max_tx_ = value.created_tx;
        }
        return false;
      });
      co_await wal_writer_->SetPersistedTx(max_tx_);
    }
  }
}

Awaitable<std::optional<std::string>> Table::Lookup(const int64_t tx, const std::string& key) {
  std::cerr << "Lookup with tx: " << tx << std::endl;
  VersionedValue result{};
  co_await lsm_.Get(key, [this, &tx, &result](const auto& data) {
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
