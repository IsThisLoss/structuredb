#include <string>

#include "utils.hpp"

namespace structuredb::server::rpc {

::grpc::Status MakeInternalError(const std::string& msg) {
  return ::grpc::Status{::grpc::StatusCode::INTERNAL, msg};
}

::grpc::Status MakeReadOnlyError() {
  return ::grpc::Status{::grpc::StatusCode::FAILED_PRECONDITION, "replica is read-only"};
}

}

