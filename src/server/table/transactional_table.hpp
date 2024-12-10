#pragma once

#include "table.hpp"

#include <transaction/storage.hpp>

namespace structuredb::server::table {

class TransactionalTable : public Table {
public:
  using Ptr = std::shared_ptr<TransactionalTable>;

  explicit TransactionalTable(LsmStorage::Ptr lsm_storage, transaction::Storage::Ptr tx_storage, transaction::TransactionId tx);

  Awaitable<void> Upsert(
      const std::string& key,
      const std::string& value
  ) override;

  Awaitable<std::optional<std::string>> Lookup(const std::string& key) override;

  Awaitable<bool> Delete(const std::string& key) override;

  Awaitable<std::vector<std::pair<std::string, std::string>>> GetAll() override;
private:
  LsmStorage::Ptr lsm_storage_;
  transaction::Storage::Ptr tx_storage_;
  transaction::TransactionId tx_;
};

}
