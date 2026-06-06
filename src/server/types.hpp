#pragma once

#include <cstdint>

namespace structuredb::server::types {

/// @brief Monotonic write sequence number (a.k.a. log sequence number).
/// Shared across storage engines, the WAL and table layers; not tied to LSM.
using Sequence = std::int64_t;

}
