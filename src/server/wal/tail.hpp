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

/// @brief collects stable WAL pages with position >= @p from
///
/// Only completed segments are returned: the highest-numbered segment is
/// assumed to be the one currently being written and is skipped so that
/// partially flushed pages are never shipped to followers. Pages are
/// returned in ascending position order.
Awaitable<std::vector<WalPageData>> CollectStablePages(
    io::Manager& io_manager,
    const std::string& wal_dir_path,
    Position from
);

}
