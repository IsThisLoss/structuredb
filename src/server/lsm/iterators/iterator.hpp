#pragma once

#include <memory>

#include <io/types.hpp>

#include <lsm/types.hpp>

namespace structuredb::server::lsm {

/// @brief LSM tree iterator interface
class Iterator {
public:
  using Ptr = std::shared_ptr<Iterator>;

  /// @brief returns true if there are records to read
  virtual bool HasMore() const = 0;;

  /// @brief returns next record
  virtual Awaitable<Record> Next() = 0;

  virtual ~Iterator() = default;
};

}
