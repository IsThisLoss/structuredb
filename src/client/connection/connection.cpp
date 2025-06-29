#include "connection.hpp"

#include <grpcpp/grpcpp.h>

namespace structuredb::client::connection {

  Connection::Connection(
    std::shared_ptr<grpc::Channel> channel,
    std::unique_ptr<structuredb::v1::Tables::Stub> tables,
    std::unique_ptr<structuredb::v1::Transactions::Stub> transactions
  ) : channel(std::move(channel)),
    tables(std::move(tables)),
    transactions(std::move(transactions)) {}

Connection::Ptr MakeConnection(const std::string& addr) {
    auto channel = grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
    auto tables = structuredb::v1::Tables::NewStub(channel);
    auto transactions = structuredb::v1::Transactions::NewStub(channel);

    return std::make_shared<Connection>(
        std::move(channel),
        std::move(tables),
        std::move(transactions)
    );
}

}
