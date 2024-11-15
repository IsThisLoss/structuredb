#include"table_service.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>

#include <wal/events/insert_event.hpp>
#include <wal/recovery.hpp>

namespace structuredb::server::services {

TableServiceImpl::TableServiceImpl(
      io::Manager& io_manager
) : io_manager_{io_manager},
    lsm_{io_manager_},
    wal_writer_{nullptr}
{
  io_manager_.CoSpawn([this]() -> Awaitable<void> {
      const std::string path = "/tmp/wal.sdb";
      co_await wal::Recover(io_manager_, path, lsm_);
      wal_writer_ = std::make_unique<wal::Writer>(co_await wal::Open(io_manager_, path));
  });
}

grpc::ServerUnaryReactor* TableServiceImpl::Upsert(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::UpsertTableRequest* request,
    ::structuredb::v1::UpsertTableResponse* response) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([this, reactor, request = *request, response]() -> Awaitable<void> {
      // std::unique_lock lock{mu_};
      co_await lsm_.Put(request.key(), request.value());
      co_await wal_writer_->Write(std::make_unique<wal::InsertEvent>(request.key(), request.value()));
      reactor->Finish(grpc::Status::OK);
  });

  return reactor;
}

grpc::ServerUnaryReactor* TableServiceImpl::Lookup(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::LookupTableRequest* request,
    ::structuredb::v1::LookupTableResponse* response) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([this, reactor, request = *request, response]() -> Awaitable<void> {
    // std::unique_lock lock{mu_};
    const auto value = co_await lsm_.Get(request.key());
    if (value.has_value()) {
      response->set_value(value.value());
    }
    reactor->Finish(grpc::Status::OK);
  });

  return reactor;
}

std::unique_ptr<grpc::Service> MakeService(
  io::Manager& io_manager
) {
  return std::make_unique<TableServiceImpl>(io_manager);
}

}
