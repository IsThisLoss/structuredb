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
    Position from
);

}
