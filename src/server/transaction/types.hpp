#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>

namespace structuredb::server::transaction {

using TransactionId = boost::uuids::uuid;

TransactionId GenerateTransactionId();

std::string ToBinary(const TransactionId& uuid);

std::string ToString(const TransactionId& uuid);

TransactionId FromString(const std::string& tx);

}
