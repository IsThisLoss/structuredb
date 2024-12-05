#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>

namespace structuredb::server::utils {

using Uuid = boost::uuids::uuid;

Uuid GenerateUuid();

std::string ToBinary(const Uuid& uuid);

std::string ToString(const Uuid& uuid);

Uuid FromString(const std::string& uuid);

}
