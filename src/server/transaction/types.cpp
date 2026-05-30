#include "types.hpp"

#include <cstring>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <string>

namespace structuredb::server::transaction {

std::string ToBinary(const TransactionId& tx) {
  std::string binary(sizeof(TransactionId), '\0');
  std::memcpy(binary.data(), &tx, sizeof(TransactionId));
  return binary;
}

std::string ToString(const TransactionId& tx) {
  return std::to_string(tx);
}

TransactionId FromString(const std::string& tx) {
  return std::stoll(tx);
}

}
