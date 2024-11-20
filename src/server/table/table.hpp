#pragma once

#include "logged_table.hpp"

#include <transaction/storage.hpp>

namespace structuredb::server::table {

class Table {
public:
  using Ptr = std::shared_ptr<Table>;

  explicit Table(LoggedTable::Ptr logged_table, transaction::Storage::Ptr tx_storage);

  Awaitable<void> Init();

  void StartLogInto(wal::Writer::Ptr wal_writer);

  Awaitable<void> RecoverFromLog(
      const lsm::Sequence seq_no,
      const std::string& key,
      const std::string& value
  );

  Awaitable<void> Upsert(
      const transaction::TransactionId& tx,
      const std::string& key,
      const std::string& value
  );

  Awaitable<std::optional<std::string>> Lookup(const transaction::TransactionId& tx, const std::string& key);
private:
  LoggedTable::Ptr logged_table_;
  transaction::Storage::Ptr tx_storage_;
};

}
