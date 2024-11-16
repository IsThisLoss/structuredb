#include "table.hpp"

#include <iostream>

#include <wal/events/insert_event.hpp>

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

Table::Table(io::Manager& io_manager, const std::string& base_dir)
  : lsm_{io_manager, base_dir}
{}

void Table::StartWal(wal::Writer::Ptr wal_writer) {
  wal_writer_ = std::move(wal_writer);
  std::cerr << "Wal attahced: " << wal_writer_ << std::endl;
}

Awaitable<void> Table::Upsert(const int64_t tx, const std::string& key, const std::string& value) {
  const VersionedValue versioned_value{
    .value = value,
    .created_tx = tx,
  };
  co_await lsm_.Put(key, ToString(versioned_value));
  if (wal_writer_) {
    co_await wal_writer_->Write(std::make_unique<wal::InsertEvent>(tx, key, value));
  }
}

Awaitable<std::optional<std::string>> Table::Lookup(const int64_t tx, const std::string& key) {
  VersionedValue result{};
  co_await lsm_.Get(key, [&tx, &result](const auto& data) {
      const auto value = ParseVersionedValue(data);
      if (value.deleted_tx < tx || tx < value.created_tx) {
        return;
      }
      if (value.created_tx > result.created_tx) {
        result = value;
      }
  });
  if (result.created_tx == 0) {
    co_return std::nullopt;
  }
  co_return std::make_optional(std::move(result.value));
}

}

