#pragma once

#include <memory>

#include <io/types.hpp>

#include "types.hpp"

namespace structuredb::server::lsm {

class Iterator {
public:
  using Ptr = std::shared_ptr<Iterator>;

  virtual bool HasMore() const = 0;;

  virtual Awaitable<Record> Next() = 0;

  ~Iterator() = default;
};

}
