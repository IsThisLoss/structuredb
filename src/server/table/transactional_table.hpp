#pragma once

#include "table.hpp"

#include <optional>
#include <transaction/storage.hpp>
#include <table/storage/storage.hpp>

namespace structuredb::server::table {

/// @brief implements table interface with MVCC transaction model and LsmStorage as undelying storage
class TransactionalTable : public Table {
public:
  using Ptr = std::shared_ptr<TransactionalTable>;

  explicit TransactionalTable(storage::Storage::Ptr table_storage, transaction::Storage::Ptr tx_storage, transaction::TransactionId tx);

  Awaitable<void> Upsert(
      const std::string& key,
      const std::string& value
  ) override;

  Awaitable<std::optional<std::string>> Lookup(const std::string& key) override;

  Awaitable<bool> Delete(const std::string& key) override;

  Awaitable<Iterator::Ptr> Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound) override;

  Awaitable<void> Compact() override;
private:
  storage::Storage::Ptr table_storage_;
  transaction::Storage::Ptr tx_storage_;
  transaction::TransactionId tx_;
};

}
