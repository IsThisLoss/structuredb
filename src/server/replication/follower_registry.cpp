#include "follower_registry.hpp"

#include <algorithm>

namespace structuredb::server::replication {

FollowerRegistry::Token FollowerRegistry::Register(wal::Position from) {
  const auto token = next_token_++;
  positions_.emplace(token, from);
  return token;
}

void FollowerRegistry::Update(Token token, wal::Position next_needed) {
  auto it = positions_.find(token);
  if (it != positions_.end()) {
    it->second = next_needed;
  }
}

void FollowerRegistry::Unregister(Token token) {
  positions_.erase(token);
}

std::optional<int64_t> FollowerRegistry::MinRetainedSegment() const {
  if (positions_.empty()) {
    return std::nullopt;
  }
  int64_t min_segment = positions_.begin()->second.segment_no;
  for (const auto& [token, pos] : positions_) {
    min_segment = std::min(min_segment, pos.segment_no);
  }
  return min_segment;
}

}
