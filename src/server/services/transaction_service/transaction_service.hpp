#pragma once

#include <transaction_service.grpc.pb.h>

#include <io/manager.hpp>
#include <database/database.hpp>

namespace structuredb::server::services {

class TransactionServiceImpl : public ::structuredb::v1::Transactions::CallbackService {
public:
  explicit TransactionServiceImpl(io::Manager& io_manager, database::Database& db);

  grpc::ServerUnaryReactor* Begin(
      grpc::CallbackServerContext* context,
      const ::structuredb::v1::BeginRequest* request,
      ::structuredb::v1::BeginResponse* response
  ) override;

  grpc::ServerUnaryReactor* Commit(
      grpc::CallbackServerContext* context,
      const ::structuredb::v1::CommitRequest* request,
      ::structuredb::v1::CommitResponse* response
  ) override;

private:
  io::Manager& io_manager_;
  database::Database& database_;
};

std::unique_ptr<grpc::Service> MakeTransactionService(
  io::Manager& io_manager,
  database::Database& db
);

}

