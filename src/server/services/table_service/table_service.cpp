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

      const auto table = co_await database_.GetTable(tx, request->table());
      if (!table) {
          reactor->Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "No such table"));
          co_return;
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

      const auto table = co_await database_.GetTable(tx, request->table());
      if (!table) {
          reactor->Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "No such table"));
          co_return;
      }

      const auto value = co_await table->Lookup(
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

grpc::ServerUnaryReactor* TableServiceImpl::CreateTable(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::CreateTableRequest* request,
    ::structuredb::v1::CreateTableResponse* response
) {
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

      try {
        co_await database_.CreateTable(tx, request->name());
      } catch (const std::exception& e) {
          reactor->Finish(grpc::Status(grpc::StatusCode::INTERNAL, e.what()));
          co_return;
      }

      if (!request->has_tx()) {
        co_await tx_storage->Commit(tx);
      }
    reactor->Finish(grpc::Status::OK);
  });

  return reactor;
}

grpc::ServerUnaryReactor* TableServiceImpl::DropTable(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::DropTableRequest* request,
    ::structuredb::v1::DropTableResponse* response
) {
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

      try {
        co_await database_.DropTable(tx, request->name());
      } catch (const std::exception& e) {
          reactor->Finish(grpc::Status(grpc::StatusCode::INTERNAL, e.what()));
          co_return;
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
