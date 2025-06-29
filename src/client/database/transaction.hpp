#pragma once

#include <database/database.hpp>
#include <connection/connection.hpp>

namespace structuredb::client::database {

class Transaction : public Database {
public:
  using Ptr = std::shared_ptr<Transaction>;

  explicit Transaction() = default;

  Transaction(const Transaction&) = delete;

  virtual void Commit() = 0;

  virtual void Rollback() = 0;
};

Transaction::Ptr MakeTransaction(connection::Connection::Ptr connection, std::string tx);

}
