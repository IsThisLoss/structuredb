#include "transactional_table.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <spdlog/spdlog.h>

#include <sdb/buffer_reader.hpp>
#include <sdb/buffer_writer.hpp>

#include <table/storage/compaction_strategy.hpp>

namespace structuredb::server::table {

namespace {

struct TransactionalValue {
  transaction::TransactionId tx{};
  bool is_deleted{};
  bool is_frozen{};
  std::string value{};
};

void Read(sdb::Reader& reader, TransactionalValue& value) {
  reader.Read(reinterpret_cast<char*>(&value.tx), sizeof(value.tx));
  value.is_deleted = reader.ReadBool();
  value.is_frozen = reader.ReadBool();
  value.value = reader.ReadString();
}

void Write(sdb::Writer& writer, const TransactionalValue& value) {
  writer.Write(reinterpret_cast<const char*>(&value.tx), sizeof(value.tx));
  writer.WriteBool(value.is_deleted);
  writer.WriteBool(value.is_frozen);
  writer.WriteString(value.value);
}

std::string ToString(const TransactionalValue& value) {
  sdb::BufferWriter writer{};
  Write(writer, value);
  auto data = std::move(writer).Extract();
  return std::string{data.data(), data.size()};
}

TransactionalValue ParseTransactionalValue(const std::string& data) {
  TransactionalValue result{};
  sdb::BufferReader reader{std::vector<char>(data.begin(), data.end())};
  Read(reader, result);
  return result;
}

class CompactStrategy : public storage::CompactionStrategy {
public:
  explicit CompactStrategy(transaction::Storage::Ptr tx_storage)
    : tx_storage_{std::move(tx_storage)}
  {}

  Awaitable<void> CompactRows(Iterator::Ptr input, OutputIterator::Ptr output) override {
    std::optional<Row> last_added;
    std::optional<std::string> last_status;
    while (input->HasMore()) {
      auto row = co_await input->Next();
      auto value = ParseTransactionalValue(row.value);
      if (value.is_deleted) {
        continue;
      }

      const auto status = co_await tx_storage_->GetStatus(value.tx);
      if (status != "commited" && status != "started") {
        continue;
      }

      if (status == "commited" && !value.is_frozen) {
        value.is_frozen = true;
        row.value = ToString(value);
      }

      if (!last_added.has_value() || last_added.value().key != row.key) {
        co_await output->Write(row);
        last_added = std::move(row);
        last_status = std::move(status);
      }
    }
  }

private:
  transaction::Storage::Ptr tx_storage_;
};

class TransactionalTableIterator : public Iterator {
public:
  explicit TransactionalTableIterator(std::vector<Row> rows)
    : rows_{std::move(rows)}
  {}

  bool HasMore() override {
    return idx_ < rows_.size();
  }

  Awaitable<Row> Next() override {
    co_return rows_[idx_++];
  }

private:
  std::vector<Row> rows_;
  size_t idx_{0};
};

}

TransactionalTable::TransactionalTable(storage::StorageEngine::Ptr table_storage, transaction::Storage::Ptr tx_storage, transaction::TransactionId tx)
  : table_storage_{std::move(table_storage)}, tx_storage_{std::move(tx_storage)}, tx_{std::move(tx)}
{}

Awaitable<void> TransactionalTable::Upsert(
      const std::string& key,
      const std::string& value
) {
  // Acquire row-level exclusive lock
  co_await tx_storage_->AcquireRowLock(tx_, key);

  auto transactional_value = ToString(TransactionalValue{
    .tx = tx_,
    .value = value,
  });
  co_await table_storage_->Upsert(Row{
    .key = key,
    .value = std::move(transactional_value),
  });
}

Awaitable<std::optional<std::string>> TransactionalTable::Lookup(const std::string& key) {
  SPDLOG_DEBUG("Lookup: tx = {}, key = {}", transaction::ToString(tx_), key);
  std::vector<TransactionalValue> candidates{};
  auto iterator = co_await table_storage_->Scan(key);

  while (iterator->HasMore()) {
    const auto record = co_await iterator->Next();
    auto candidate = ParseTransactionalValue(record.value);
    SPDLOG_DEBUG("Lookup candidate: tx = {}, value = {}", transaction::ToString(candidate.tx), candidate.value);
    if (candidate.is_frozen || candidate.tx == tx_ || co_await tx_storage_->IsCommited(candidate.tx)) {
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

  // Acquire row-level exclusive lock
  co_await tx_storage_->AcquireRowLock(tx_, key);

  auto transactional_value = ToString(TransactionalValue{
    .tx = tx_,
    .is_deleted = true,
  });
  co_await table_storage_->Upsert(Row{
    .key = key,
    .value = std::move(transactional_value),
  });
  co_return true;
}

Awaitable<Iterator::Ptr>
TransactionalTable::Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound) {
  std::vector<Row> result;
  auto iterator = co_await table_storage_->Scan(lower_bound, upper_bound);
  while (iterator->HasMore()) {
    auto record = co_await iterator->Next();
    auto candidate = ParseTransactionalValue(record.value);
    if (candidate.tx == tx_ || co_await tx_storage_->IsCommited(candidate.tx)) {
      if (candidate.is_deleted) {
        continue;
      }
      if (!result.empty() && result.back().key == record.key) {
        continue;
      }
      result.emplace_back(std::move(record.key), std::move(candidate.value));
    }
  }
  co_return std::make_shared<TransactionalTableIterator>(std::move(result));
}

Awaitable<void> TransactionalTable::Compact() {
  co_await table_storage_->Compact(std::make_shared<CompactStrategy>(tx_storage_));
}

}
