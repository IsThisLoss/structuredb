#include "uuid.hpp"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <spdlog/spdlog.h>

namespace structuredb::server::utils {

namespace {

thread_local boost::uuids::random_generator random_generator;
thread_local boost::uuids::string_generator string_generator;

}

Uuid GenerateUuid() {
  return random_generator();
}

std::string ToBinary(const Uuid& uuid) {
  std::string result;
  result.resize(uuid.size());
  ::memcpy(result.data(), uuid.data, uuid.size());
  return result;
}

std::string ToString(const Uuid& uuid) {
  return boost::uuids::to_string(uuid);
}

Uuid FromString(const std::string& uuid) {
  return string_generator(uuid);
}

}

