#include "transaction.hpp"

#include <utils/status_check.hpp>

namespace structuredb::client::database {

class TransactionImpl : public Transaction {
public:
  explicit TransactionImpl(connection::Connection::Ptr connection, std::string tx)
    : connection_{std::move(connection)}, tx_{std::move(tx)}
  {}

  void CreateTable(const std::string& table_name) const override {
    v1::CreateTableRequest req{};
    req.set_name(table_name);
    req.set_tx(tx_);

    v1::CreateTableResponse res{};
    const auto status = connection_->tables->CreateTable(&connection_->context, req, &res);
    CheckStatus(status);
  }

  void DropTable(const std::string& table_name) const override {
    v1::DropTableRequest req{};
    req.set_name(table_name);
    req.set_tx(tx_);

    v1::DropTableResponse res{};
    const auto status = connection_->tables->DropTable(&connection_->context, req, &res);
    CheckStatus(status);
  }

  table::TableClient::Ptr Table(const std::string& table_name) const override {
    return table::MakeTableClient(connection_, table_name, tx_);
  }

  void Commit() override {
    v1::CommitRequest req{};
    req.set_tx(tx_);

    v1::CommitResponse res{};
    const auto status = connection_->transactions->Commit(&connection_->context, req, &res);
    CheckStatus(status);
  }

  void Rollback() override {
    // TODO
  }

private:
  const connection::Connection::Ptr connection_;
  const std::string tx_;
};

Transaction::Ptr MakeTransaction(connection::Connection::Ptr connection, std::string tx) {
  return std::make_shared<TransactionImpl>(std::move(connection), std::move(tx));
}

}

