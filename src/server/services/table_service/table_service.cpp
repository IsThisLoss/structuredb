#include"table_service.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>

namespace structuredb::server::services {

TableServiceImpl::TableServiceImpl(boost::asio::io_context& io_context)
  : io_context_{io_context},
    lsm_{io_context_}
{}

grpc::ServerUnaryReactor* TableServiceImpl::Upsert(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::UpsertTableRequest* request,
    ::structuredb::v1::UpsertTableResponse* response) {
  std::cerr << "Staring table service upsert" << std::endl;
  auto* reactor = context->DefaultReactor();

  boost::asio::co_spawn(io_context_, [&]() -> boost::asio::awaitable<void> {
      std::cerr << "Staring upsert in coroutine" << std::endl;
      std::unique_lock lock{mu_};
      co_await lsm_.Put(request->key(), request->value());
      reactor->Finish(grpc::Status::OK);
      std::cerr << "Finish upsert in coroutine" << std::endl;
  }, boost::asio::detached);

  std::cerr << "Return reactor" << std::endl;
  return reactor;
}

grpc::ServerUnaryReactor* TableServiceImpl::Lookup(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::LookupTableRequest* request,
    ::structuredb::v1::LookupTableResponse* response) {
  {
    std::unique_lock lock{mu_};
    const auto value = lsm_.Get(request->key());
    if (value.has_value()) {
      response->set_value(value.value());
    }
  }

  auto* reactor = context->DefaultReactor();
  reactor->Finish(grpc::Status::OK);
  return reactor;
}

std::unique_ptr<grpc::Service> MakeService(boost::asio::io_context& io_context) {
  return std::make_unique<TableServiceImpl>(io_context);
}

}
