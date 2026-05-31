#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <sdb/reader.hpp>
#include <sdb/writer.hpp>

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

void Write(sdb::Writer& writer, const Record& record);

void Read(sdb::Reader& reader, Record& record);

struct ScanRange {
  std::optional<std::string> lower_bound{std::nullopt};
  std::optional<std::string> upper_bound{std::nullopt};
  
  static ScanRange FullScan();
};



}
