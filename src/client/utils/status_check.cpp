#include "status_check.hpp"

namespace structuredb::client {

void CheckStatus(const grpc::Status& status) {
  if (!status.ok()) {
    throw std::runtime_error{status.error_message()};
  }
}

}
