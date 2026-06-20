#pragma once

#include <string>

#include <io/manager.hpp>
#include <database/database.hpp>

namespace structuredb::server::replication {

/// @brief follower-side replication client
///
/// Streams WAL pages from the leader, mirrors them into the local WAL
/// directory and applies them through the recovery path. Mirroring keeps the
/// follower's durability model identical to a standalone node: on restart the
/// normal WAL recovery replays the received pages, and streaming resumes from
/// the first segment not yet persisted locally.
class Follower {
public:
  explicit Follower(
      io::Manager& io_manager,
      database::Database& db,
      std::string leader_address,
      std::string wal_dir_path
  );

  /// @brief blocking reconnect loop; intended to run on a dedicated thread
  void Run();

private:
  io::Manager& io_manager_;
  database::Database& db_;
  const std::string leader_address_;
  const std::string wal_dir_path_;
};

}
