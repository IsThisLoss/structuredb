#pragma once

#include <functional>
#include <string>

#include <io/types.hpp>

namespace structuredb::server::lsm {

using RecordConsumer = std::function<void(const std::string)>;

}
