#include "autocommit_database.hpp"

#include <memory>
#include <string>
#include <table_service.pb.h>

#include <utils/status_check.hpp>

namespace structuredb::client::database {

class AutoCommitDatabaseImpl : public AutoCommitDatabase {
public:
  explicit AutoCommitDatabaseImpl(connection::Connection::Ptr connection)
    : connection_{std::move(connection)}
  {}

  void CreateTable(const std::string& table_name) const override {
    v1::CreateTableRequest req{};
    req.set_name(table_name);

    v1::CreateTableResponse res{};
    grpc::ClientContext context{};
    const auto status = connection_->tables->CreateTable(&context, req, &res);
    CheckStatus(status);
  }

  void DropTable(const std::string& table_name) const override {
    v1::DropTableRequest req{};
    req.set_name(table_name);

    v1::DropTableResponse res{};
    grpc::ClientContext context{};
    const auto status = connection_->tables->DropTable(&context, req, &res);
    CheckStatus(status);
  }

  table::TableClient::Ptr Table(const std::string& table_name) const override {
    return table::MakeTableClient(connection_, table_name, std::nullopt);
  }

  Transaction::Ptr Begin() const override {
    v1::BeginRequest req{};
    v1::BeginResponse resp{};
    grpc::ClientContext context{};
    connection_->transactions->Begin(&context, req, &resp);
    return MakeTransaction(connection_, resp.tx());
  }

  void WithTransaction(const TxCallback& cb) const override {
    auto tx = Begin();
    try {
      cb(tx);
    } catch (const std::exception& e) {
      tx->Rollback();
      throw;
    }
    tx->Commit();
  }

private:
  connection::Connection::Ptr connection_;
};

AutoCommitDatabase::Ptr MakeAutoCommitDatabase(connection::Connection::Ptr connection) {
  return std::make_shared<AutoCommitDatabaseImpl>(std::move(connection));
}

}
