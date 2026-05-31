#pragma once

#include <memory>
#include <string>
#include <table_service.grpc.pb.h>
#include <transaction_service.grpc.pb.h>

namespace structuredb::client::connection {

struct Connection {
  using Ptr = std::shared_ptr<Connection>;

  std::shared_ptr<grpc::Channel> channel;
  std::unique_ptr<structuredb::v1::Tables::Stub> tables;
  std::unique_ptr<structuredb::v1::Transactions::Stub> transactions;

  explicit Connection(
    std::shared_ptr<grpc::Channel> channel,
    std::unique_ptr<structuredb::v1::Tables::Stub> tables,
    std::unique_ptr<structuredb::v1::Transactions::Stub> transactions
  );
};

Connection::Ptr MakeConnection(const std::string& addr);

}
