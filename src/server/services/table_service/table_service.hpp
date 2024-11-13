#pragma once

#include <table_service.grpc.pb.h>

#include <lsm/lsm.hpp>

namespace structuredb::server::services {

class TableServiceImpl : public ::structuredb::v1::Tables::CallbackService {
public:
  explicit TableServiceImpl(boost::asio::io_context& io_context);

  grpc::ServerUnaryReactor* Upsert(
      grpc::CallbackServerContext* context,
      const ::structuredb::v1::UpsertTableRequest* request,
      ::structuredb::v1::UpsertTableResponse* response
  ) override;

  grpc::ServerUnaryReactor* Lookup(
      grpc::CallbackServerContext* context,
      const ::structuredb::v1::LookupTableRequest* request,
      ::structuredb::v1::LookupTableResponse* response
  ) override;

private:
  boost::asio::io_context& io_context_;
  std::mutex mu_;
  lsm::Lsm lsm_;
};

std::unique_ptr<grpc::Service> MakeService(boost::asio::io_context& io_context);

}
