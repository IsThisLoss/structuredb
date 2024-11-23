#include "types.hpp"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace structuredb::server::transaction {

namespace {

thread_local boost::uuids::random_generator random_generator;
thread_local boost::uuids::string_generator string_generator;

}

TransactionId GenerateTransactionId() {
  return random_generator();
}

std::string ToBinary(const TransactionId& uuid) {
  std::string result;
  result.resize(uuid.size());
  ::memcpy(result.data(), uuid.data, uuid.size());
  return result;
}

std::string ToString(const TransactionId& uuid) {
  const auto result = boost::uuids::to_string(uuid);
  return result;
}

TransactionId FromString(const std::string& tx) {
  return string_generator(tx);
}

}
