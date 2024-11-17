#pragma once

#include <functional>
#include <string>

#include <io/types.hpp>

namespace structuredb::server::lsm {

using RecordConsumer = std::function<bool(const std::string)>;

}
