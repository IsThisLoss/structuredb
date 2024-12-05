#pragma once

#include <string>

#include <utils/uuid.hpp>

namespace structuredb::server::transaction {

using TransactionId = utils::Uuid;

TransactionId GenerateTransactionId();

std::string ToBinary(const TransactionId& uuid);

std::string ToString(const TransactionId& uuid);

TransactionId FromString(const std::string& tx);

}
