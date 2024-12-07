#pragma once

#include <grpc/status.h>
#include <grpcpp/impl/status.h>

#include <transaction/types.hpp>

namespace structuredb::server::rpc {

template <typename Msg>
std::optional<transaction::TransactionId> ParseTx(const Msg* msg) {
  if (!msg->has_tx()) {
    return std::nullopt;
  }
  return transaction::FromString(msg->tx());
}


::grpc::Status MakeInternalError(const std::string& msg);

}
