#pragma once

#include <cstdint>
#include <string>

namespace structuredb::server::wal {

int64_t GetSegmentNoFromName(const std::string& name);

}
