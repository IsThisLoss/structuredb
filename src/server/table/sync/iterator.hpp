#pragma once

#include <io/manager.hpp>
#include <table/iterator.hpp>

namespace structuredb::server::table::sync {

/// @brief sync interface of iterator
class Iterator {
public:
  using Ptr = std::shared_ptr<Iterator>;

  explicit Iterator(io::Manager& io_manager, table::Iterator::Ptr impl);

  bool HasMore();

  Row Next();
private:
  io::Manager& io_manager_;
  table::Iterator::Ptr impl_;
};

}
