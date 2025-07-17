#pragma once

#include <database/autocommit_database.hpp>

namespace structuredb::cli {

class Executor {
public:
  explicit Executor(client::database::AutoCommitDatabase::Ptr db_);

  void Execute(const std::string_view& command);

private:
  client::database::AutoCommitDatabase::Ptr db_;
  client::database::Transaction::Ptr tx_;

  void EnsureInTransaction() const;
};

} // namespace structuredb::cli
