#include"table_service.hpp"

#include <rpc/utils.hpp>

namespace structuredb::server::services {

namespace {

}

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

  io_manager_.CoSpawn([this, request, response, reactor]() -> Awaitable<void> {
      std::unique_lock lock{mu_};

      try {
        const auto tx = rpc::ParseTx(request);
        auto session = co_await database_.StartSession(tx);
        auto table = co_await session.GetTable(request->table());
        if (!table) {
            reactor->Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "No such table"));
            co_return;
        }

        co_await table->Upsert(
            request->key(),
            request->value()
        );

        auto result_tx = co_await session.Finish();
        response->set_tx(transaction::ToString(result_tx));
        reactor->Finish(grpc::Status::OK);
      } catch (const std::exception& e) {
        reactor->Finish(rpc::MakeInternalError(e.what()));
      }
  });

  return reactor;
}

grpc::ServerUnaryReactor* TableServiceImpl::Lookup(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::LookupTableRequest* request,
    ::structuredb::v1::LookupTableResponse* response) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([this, reactor, request, response]() -> Awaitable<void> {
      std::unique_lock lock{mu_};

      try {
        const auto tx = rpc::ParseTx(request);
        auto session = co_await database_.StartSession(tx);
        auto table = co_await session.GetTable(request->table());
        if (!table) {
            reactor->Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "No such table"));
            co_return;
        }

        auto value = co_await table->Lookup(
            request->key()
        );
        if (value.has_value()) {
          response->set_value(value.value());
        }

        auto result_tx = co_await session.Finish();
        reactor->Finish(grpc::Status::OK);
      } catch (const std::exception& e) {
        reactor->Finish(rpc::MakeInternalError(e.what()));
      }
  });

  return reactor;
}

grpc::ServerUnaryReactor* TableServiceImpl::Delete(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::DeleteTableRequest* request,
    ::structuredb::v1::DeleteTableResponse* response
) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([this, request, response, reactor]() -> Awaitable<void> {
      std::unique_lock lock{mu_};

      try {
        const auto tx = rpc::ParseTx(request);
        auto session = co_await database_.StartSession(tx);
        auto table = co_await session.GetTable(request->table());
        if (!table) {
            reactor->Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "No such table"));
            co_return;
        }

        co_await table->Delete(
            request->key()
        );

        auto result_tx = co_await session.Finish();
        response->set_tx(transaction::ToString(result_tx));
        reactor->Finish(grpc::Status::OK);
      } catch (const std::exception& e) {
        reactor->Finish(rpc::MakeInternalError(e.what()));
      }
  });

  return reactor;
}

grpc::ServerUnaryReactor* TableServiceImpl::CreateTable(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::CreateTableRequest* request,
    ::structuredb::v1::CreateTableResponse* response
) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([this, request, response, reactor]() -> Awaitable<void> {
      std::unique_lock lock{mu_};

      try {
        const auto tx = rpc::ParseTx(request);
        auto session = co_await database_.StartSession(tx);

        co_await session.CreateTable(
            request->name()
        );

        auto result_tx = co_await session.Finish();
        response->set_tx(transaction::ToString(result_tx));
        reactor->Finish(grpc::Status::OK);
      } catch (const std::exception& e) {
        reactor->Finish(rpc::MakeInternalError(e.what()));
      }
  });

  return reactor;
}

grpc::ServerUnaryReactor* TableServiceImpl::DropTable(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::DropTableRequest* request,
    ::structuredb::v1::DropTableResponse* response
) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([this, request, response, reactor]() -> Awaitable<void> {
      std::unique_lock lock{mu_};

      try {
        const auto tx = rpc::ParseTx(request);
        auto session = co_await database_.StartSession(tx);

        co_await session.DropTable(
            request->name()
        );

        auto result_tx = co_await session.Finish();
        response->set_tx(transaction::ToString(result_tx));
        reactor->Finish(grpc::Status::OK);
      } catch (const std::exception& e) {
        reactor->Finish(rpc::MakeInternalError(e.what()));
      }
  });

  return reactor;
}

grpc::ServerUnaryReactor* TableServiceImpl::Scan(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::ScanTableRequest* request,
    ::structuredb::v1::ScanTableResponse* response
) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([this, request, response, reactor]() -> Awaitable<void> {
      std::unique_lock lock{mu_};

      try {
        const auto tx = rpc::ParseTx(request);
        auto session = co_await database_.StartSession(tx);
        auto table = co_await session.GetTable(request->table());
        if (!table) {
            reactor->Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "No such table"));
            co_return;
        }

        std::optional<std::string> lower_bound{};
        if (request->has_lower_bound()) {
          lower_bound = request->lower_bound();
        }
        std::optional<std::string> upper_bound{};
        if (request->has_upper_bound()) {
          upper_bound = request->upper_bound();
        }

        const auto records = co_await table->Scan(lower_bound, upper_bound);

        for (const auto& [key, value] : records) {
          auto* record = response->add_records();
          record->set_key(key);
          record->set_value(value);
        }

        auto result_tx = co_await session.Finish();
        response->set_tx(transaction::ToString(result_tx));
        reactor->Finish(grpc::Status::OK);
      } catch (const std::exception& e) {
        reactor->Finish(rpc::MakeInternalError(e.what()));
      }
  });

  return reactor;
}

grpc::ServerUnaryReactor* TableServiceImpl::CompactTable(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::CompactTableRequest* request,
    ::structuredb::v1::CompactTableResponse* response
) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([this, request, response, reactor]() -> Awaitable<void> {
      std::unique_lock lock{mu_};

      try {
        const auto tx = rpc::ParseTx(request);
        auto session = co_await database_.StartSession(tx);
        co_await session.CompactTable(request->table());
        reactor->Finish(grpc::Status::OK);
      } catch (const std::exception& e) {
        reactor->Finish(rpc::MakeInternalError(e.what()));
      }
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
