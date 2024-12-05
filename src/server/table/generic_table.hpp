#pragma once

#include "logged_table.hpp"

#include <transaction/storage.hpp>

namespace structuredb::server::table {

class Table {
public:
  using Ptr = std::shared_ptr<Table>;

  explicit Table(LoggedTable::Ptr logged_table, transaction::Storage::Ptr tx_storage, transaction::TransactionId tx);

  Awaitable<void> Upsert(
      const std::string& key,
      const std::string& value
  );

  Awaitable<std::optional<std::string>> Lookup(const std::string& key);
private:
  LoggedTable::Ptr logged_table_;
  transaction::Storage::Ptr tx_storage_;
  transaction::TransactionId tx_;
};

}
