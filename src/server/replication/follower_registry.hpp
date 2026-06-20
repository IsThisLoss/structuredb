#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

#include <wal/reader.hpp>

namespace structuredb::server::replication {

/// @brief tracks the WAL position of currently connected followers
///
/// Used for WAL retention on the leader: a segment may only be cleaned once
/// every connected follower has been streamed past it. Positions reflect what
/// has been sent (the pull protocol carries no acknowledgements), so a follower
/// that disconnects before persisting may have to re-bootstrap from a snapshot
/// — consistent with the asynchronous replication guarantees.
class FollowerRegistry {
public:
  using Ptr = std::shared_ptr<FollowerRegistry>;
  using Token = uint64_t;

  /// @brief register a follower starting at @p from; returns its token
  Token Register(wal::Position from);

  /// @brief advance the position the follower has been streamed up to
  void Update(Token token, wal::Position next_needed);

  /// @brief drop a follower (on disconnect)
  void Unregister(Token token);

  /// @brief lowest segment any connected follower still needs
  ///
  /// Returns nullopt when no followers are connected, meaning retention adds
  /// no constraint.
  std::optional<int64_t> MinRetainedSegment() const;

private:
  mutable std::mutex mutex_;
  Token next_token_{0};
  std::unordered_map<Token, wal::Position> positions_;
};

}
