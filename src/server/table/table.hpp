#pragma once

#include "logged_table.hpp"

namespace structuredb::server::table {

class Table {
public:
  using Ptr = std::shared_ptr<Table>;

  explicit Table(io::Manager& io_manager, const std::string& base_dir, database::Database& db);

  void StartLogInto(wal::Writer::Ptr wal_writer);

  Awaitable<void> RecoverRecord(
      const std::string& key,
      const lsm::Sequence seq_no,
      const std::string& value
  );

  Awaitable<void> Upsert(
      const int64_t tx,
      const std::string& key,
      const std::string& value
  );

  Awaitable<std::optional<std::string>> Lookup(const int64_t tx, const std::string& key);
private:
  database::Database& db_;
  LoggedTable logged_table_;
};

}
