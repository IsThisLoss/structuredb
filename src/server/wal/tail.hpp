#pragma once

#include <string>
#include <vector>

#include <io/manager.hpp>

#include "reader.hpp"

namespace structuredb::server::wal {

/// @brief a single WAL page together with its position
struct WalPageData {
  Position position{};
  std::vector<char> data{};
};

/// @brief returns true if @p lhs is ordered before @p rhs
bool operator<(const Position& lhs, const Position& rhs);

/// @brief collects WAL pages with position >= @p from in ascending order
///
/// Includes the current, not-yet-completed page of the in-progress segment so
/// that freshly written records ship without waiting for a segment to fill. A
/// completed page is exactly kWalPageSize bytes and immutable; a shorter page
/// is still growing and will be re-read on subsequent calls. Callers rely on
/// follower-side idempotency to re-apply a growing tail page safely.
Awaitable<std::vector<WalPageData>> CollectPagesFrom(
    io::Manager& io_manager,
    const std::string& wal_dir_path,
    const Position from
);

/// @brief lowest segment number that is missing or not yet complete
///
/// A complete segment is exactly kWalPageSize bytes and immutable; a shorter
/// (or absent) one may have been received only partially. Used by a follower
/// to decide where to resume streaming after a restart.
Awaitable<int64_t> NextSegmentToFetch(
    io::Manager& io_manager,
    const std::string& wal_dir_path
);

/// @brief writes a received raw WAL page to its segment file (truncating)
///
/// Mirrors a page streamed from the leader into the local WAL so that normal
/// recovery replays it on restart. Assumes one page per segment.
Awaitable<void> WriteReceivedPage(
    io::Manager& io_manager,
    const std::string& wal_dir_path,
    int64_t segment_no,
    const std::string& data
);

}
