#include"table_service.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>

namespace structuredb::server::services {

TableServiceImpl::TableServiceImpl(io::Manager& io_manager)
  : io_manager_{io_manager},
    lsm_{io_manager}
{}

grpc::ServerUnaryReactor* TableServiceImpl::Upsert(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::UpsertTableRequest* request,
    ::structuredb::v1::UpsertTableResponse* response) {
  std::cerr << "Staring table service upsert" << std::endl;
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([this, reactor, request = *request, response]() -> Awaitable<void> {
      // std::cerr << "Staring upsert in coroutine" << std::endl;
      /// std::unique_lock lock{mu_};
      co_await lsm_.Put(request.key(), request.value());
      reactor->Finish(grpc::Status::OK);
      co_return;
      /// std::cerr << "Finish upsert in coroutine" << std::endl;
  });

  std::cerr << "Return reactor" << std::endl;
  return reactor;
}

grpc::ServerUnaryReactor* TableServiceImpl::Lookup(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::LookupTableRequest* request,
    ::structuredb::v1::LookupTableResponse* response) {
  auto* reactor = context->DefaultReactor();

  io_manager_.CoSpawn([this, reactor, request = *request, response]() -> Awaitable<void> {
    std::unique_lock lock{mu_};
    const auto value = co_await lsm_.Get(request.key());
    if (value.has_value()) {
      response->set_value(value.value());
    }
    reactor->Finish(grpc::Status::OK);
  });

  return reactor;
}

std::unique_ptr<grpc::Service> MakeService(io::Manager& io_manager) {
  return std::make_unique<TableServiceImpl>(io_manager);
}

}
