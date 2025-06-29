#pragma once

#include <grpcpp/support/status.h>

namespace structuredb::client {

void CheckStatus(const grpc::Status& status);

}
