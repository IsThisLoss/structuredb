#pragma once

#include <cstdint>
#include <sys/types.h>

namespace structuredb::server::wal {

constexpr int64_t kWalPageSize = 4096;

constexpr int64_t kMaxPagesInSegment = 1;

}
