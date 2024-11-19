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

}
