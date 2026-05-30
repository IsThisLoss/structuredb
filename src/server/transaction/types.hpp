#pragma once

#include <cstdint>
#include <string>

#include <utils/uuid.hpp>

namespace structuredb::server::transaction {

using TransactionId = int64_t;

std::string ToBinary(const TransactionId& uuid);

std::string ToString(const TransactionId& uuid);

TransactionId FromString(const std::string& tx);

}
