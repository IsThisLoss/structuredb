#include"table_service.hpp"

namespace structuredb::server::services {

grpc::ServerUnaryReactor* TableServiceImpl::Upsert(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::UpsertTableRequest* request,
    ::structuredb::v1::UpsertTableResponse* response) {
  auto* reactor = context->DefaultReactor();
  impl_[request->key()] = request->value();
  reactor->Finish(grpc::Status::OK);
  return reactor;
}

grpc::ServerUnaryReactor* TableServiceImpl::Lookup(
    grpc::CallbackServerContext* context,
    const ::structuredb::v1::LookupTableRequest* request,
    ::structuredb::v1::LookupTableResponse* response) {
  auto* reactor = context->DefaultReactor();
  const auto it = impl_.find(request->key());
  if (it != impl_.end()) {
    response->set_value(it->second);
  }
  reactor->Finish(grpc::Status::OK);
  return reactor;
}

std::unique_ptr<grpc::Service> MakeService() {
  return std::make_unique<TableServiceImpl>();
}

}
