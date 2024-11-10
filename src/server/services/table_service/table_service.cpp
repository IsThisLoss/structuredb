#include"table_service.hpp"

namespace structuredb::server::services {

grpc::ServerUnaryReactor* TableServiceImpl::Upsert(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::UpsertTableRequest* request,
    ::structuredb::v1::UpsertTableResponse* response) {

  {
    std::unique_lock lock{mu_};
    lsm_.Put(request->key(), request->value());
  }

  auto* reactor = context->DefaultReactor();
  reactor->Finish(grpc::Status::OK);
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

std::unique_ptr<grpc::Service> MakeService() {
  return std::make_unique<TableServiceImpl>();
}

}
