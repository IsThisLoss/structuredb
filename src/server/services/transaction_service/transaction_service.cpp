#include "transaction_service.hpp"

namespace structuredb::server::services {

TransactionServiceImpl::TransactionServiceImpl(io::Manager& io_manager, database::Database& db)
  : io_manager_{io_manager}
  , database_{db}
{}

grpc::ServerUnaryReactor* TransactionServiceImpl::Begin(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::BeginRequest* request,
    ::structuredb::v1::BeginResponse* response
) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([&]() -> Awaitable<void> {
      const auto tx = co_await database_.GetTransactionStorage()->Begin();
      response->set_tx(transaction::ToString(tx));
      reactor->Finish(grpc::Status::OK);
      co_return;
  });

  return reactor;
}

grpc::ServerUnaryReactor* TransactionServiceImpl::Commit(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::CommitRequest* request,
    ::structuredb::v1::CommitResponse* response
) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([&]() -> Awaitable<void> {
      const auto tx = transaction::FromString(request->tx());
      co_await database_.GetTransactionStorage()->Commit(tx);
      reactor->Finish(grpc::Status::OK);
      co_return;
  });

  return reactor;
}

std::unique_ptr<grpc::Service> MakeTransactionService(
  io::Manager& io_manager,
  database::Database& db
) {
  return std::make_unique<TransactionServiceImpl>(io_manager, db);
}

}
