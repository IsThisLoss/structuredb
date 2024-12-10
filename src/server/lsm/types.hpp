#pragma once

#include <string>

namespace structuredb::server::lsm {

using Sequence = int64_t;

constexpr const Sequence kMaxSequence = std::numeric_limits<Sequence>::max();

struct Record {
  std::string key{};
  Sequence seq_no{kMaxSequence};
  std::string value{};

  bool operator<(const Record& rhs) const;
};

struct ScanRange {
  std::optional<std::string> lower_bound{std::nullopt};
  std::optional<std::string> upper_bound{std::nullopt};
};

}
