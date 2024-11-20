#include"table_service.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>

#include <wal/recovery.hpp>

namespace structuredb::server::services {

TableServiceImpl::TableServiceImpl(
      io::Manager& io_manager,
      database::Database& db
) : io_manager_{io_manager},
    database_{db}
{
}

grpc::ServerUnaryReactor* TableServiceImpl::Upsert(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::UpsertTableRequest* request,
    ::structuredb::v1::UpsertTableResponse* response) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([&]() -> Awaitable<void> {
      std::unique_lock lock{mu_};
      const auto table = database_.GetTable("table");
      if (!table) {
        std::cerr << "Null table" << std::endl;
      }

      auto tx_storage = database_.GetTransactionStorage();
      transaction::TransactionId tx{};
      if (request->has_tx()) {
        tx = transaction::FromString(request->tx());
        if (!co_await tx_storage->IsStarted(tx)) {
          reactor->Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid transaction id"));
          co_return;
        }
      } else {
        tx = co_await tx_storage->Begin();
      }

      co_await table->Upsert(
          tx,
          request->key(),
          request->value()
      );

      response->set_tx(transaction::ToString(tx));
      if (!request->has_tx()) {
        co_await tx_storage->Commit(tx);
      }
      reactor->Finish(grpc::Status::OK);
  });

  return reactor;
}

grpc::ServerUnaryReactor* TableServiceImpl::Lookup(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::LookupTableRequest* request,
    ::structuredb::v1::LookupTableResponse* response) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([&]() -> Awaitable<void> {
      std::unique_lock lock{mu_};

      auto tx_storage = database_.GetTransactionStorage();
      transaction::TransactionId tx{};
      if (request->has_tx()) {
        tx = transaction::FromString(request->tx());
        if (!co_await tx_storage->IsStarted(tx)) {
          reactor->Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Invalid transaction id"));
          co_return;
        }
      } else {
        tx = co_await tx_storage->Begin();
      }

      const auto value = co_await database_.GetTable("table")->Lookup(
          tx,
          request->key()
      );
      if (value.has_value()) {
        response->set_value(value.value());
      }
      if (!request->has_tx()) {
        co_await tx_storage->Commit(tx);
      }
    reactor->Finish(grpc::Status::OK);
  });

  return reactor;
}

std::unique_ptr<grpc::Service> MakeService(
  io::Manager& io_manager,
  database::Database& db
) {
  return std::make_unique<TableServiceImpl>(io_manager, db);
}

}
