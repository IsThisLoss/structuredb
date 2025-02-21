#pragma once 

#include <io/types.hpp>
#include "row.hpp"

namespace structuredb::server::table {

/// @brief interface of table iterator
class Iterator {
public:
  using Ptr = std::shared_ptr<Iterator>;

  virtual bool HasMore() = 0;

  virtual Awaitable<Row> Next() = 0;

  virtual ~Iterator() = default;
};

/// @brief interface of table writer
class OutputIterator {
public:
  using Ptr = std::shared_ptr<OutputIterator>;

  /// @brief writes row into table
  virtual Awaitable<void> Write(Row row) = 0;

  virtual ~OutputIterator() = default;
};

}
