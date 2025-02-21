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

}
