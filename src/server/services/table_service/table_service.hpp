#pragma once

#include <table_service.grpc.pb.h>

#include <lsm/lsm.hpp>
#include <wal/writer.hpp>

namespace structuredb::server::services {

class TableServiceImpl : public ::structuredb::v1::Tables::CallbackService {
public:
  explicit TableServiceImpl(io::Manager& io_manager);

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
  io::Manager& io_manager_;
  lsm::Lsm lsm_;
  std::unique_ptr<wal::Writer> wal_writer_;
};

std::unique_ptr<grpc::Service> MakeService(
  io::Manager& io_manager
);

}
