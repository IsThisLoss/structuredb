#include "transaction_service.hpp"

#include <rpc/utils.hpp>

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

  io_manager_.CoSpawn([this, request, response, reactor]() -> Awaitable<void> {
      try {
        auto session = co_await database_.StartSession();
        response->set_tx(transaction::ToString(session.GetTx()));
        // NOTE: do not call session.Finish() here
        reactor->Finish(grpc::Status::OK);
      } catch (const std::exception& e) {
        reactor->Finish(rpc::MakeInternalError(e.what()));
      }
  });

  return reactor;
}

grpc::ServerUnaryReactor* TransactionServiceImpl::Commit(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::CommitRequest* request,
    ::structuredb::v1::CommitResponse* response
) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([this, request, response, reactor]() -> Awaitable<void> {
      try {
        const auto tx = transaction::FromString(request->tx());
        auto session = co_await database_.StartSession(tx);
        co_await session.Commit();
        co_await session.Finish();
        reactor->Finish(grpc::Status::OK);
      } catch (const std::exception& e) {
        reactor->Finish(rpc::MakeInternalError(e.what()));
      }
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
