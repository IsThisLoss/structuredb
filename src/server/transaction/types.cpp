#include "types.hpp"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace structuredb::server::transaction {

TransactionId GenerateTransactionId() {
  return utils::GenerateUuid();
}

std::string ToBinary(const TransactionId& uuid) {
  return utils::ToBinary(uuid);
}

std::string ToString(const TransactionId& uuid) {
  return utils::ToString(uuid);
}

TransactionId FromString(const std::string& tx) {
  return utils::FromString(tx);
}

}
