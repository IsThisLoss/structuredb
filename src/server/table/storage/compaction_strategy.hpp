#pragma once

#include <table/iterator.hpp>

namespace structuredb::server::table::storage {

/// @brief interface of table compaction strategy
class CompactionStrategy {
public:
  using Ptr = std::shared_ptr<CompactionStrategy>;

  virtual Awaitable<void> CompactRows(Iterator::Ptr input, OutputIterator::Ptr out) = 0;

  virtual ~CompactionStrategy() = default;
};

}
