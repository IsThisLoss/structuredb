#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <io/manager.hpp>
#include <database/database.hpp>

namespace structuredb::server::wal {

/// @brief removes WAL segments that are both persisted and no longer needed
///
/// A persisted segment is retained when @p retain_from_segment is set and the
/// segment number is at or above it: such segments are still required by a
/// connected replication follower.
Awaitable<void> Clean(
    io::Manager& io_manager,
    const std::string& wal_dir_path,
    database::Database& db,
    std::optional<int64_t> retain_from_segment = std::nullopt
);

}
