#pragma once

#include <table_service.grpc.pb.h>

#include <io/manager.hpp>
#include <database/database.hpp>

namespace structuredb::server::services {

class TableServiceImpl : public ::structuredb::v1::Tables::CallbackService {
public:
  explicit TableServiceImpl(io::Manager& io_manager, database::Database& db);

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

  grpc::ServerUnaryReactor* CreateTable(
      grpc::CallbackServerContext* context,
      const ::structuredb::v1::CreateTableRequest* request,
      ::structuredb::v1::CreateTableResponse* response
  ) override;

  grpc::ServerUnaryReactor* DropTable(
      grpc::CallbackServerContext* context,
      const ::structuredb::v1::DropTableRequest* request,
      ::structuredb::v1::DropTableResponse* response
  ) override;

private:
  io::Manager& io_manager_;
  database::Database& database_;
  std::mutex mu_;

};

std::unique_ptr<grpc::Service> MakeService(
  io::Manager& io_manager,
  database::Database& db
);

}
