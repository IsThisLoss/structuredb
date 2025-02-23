#pragma once

#include <io/manager.hpp>
#include <table/table.hpp>

#include "iterator.hpp"

namespace structuredb::server::table::sync {

/// @brief sync interface of table
class Table {
public:
  using Ptr = std::shared_ptr<Table>;

 explicit Table(
    io::Manager& io_manager,
    table::Table::Ptr impl
 );

  void Upsert(
      const std::string& key,
      const std::string& value
  );

  std::optional<std::string> Lookup(const std::string& key);

  bool Delete(const std::string& key);

  sync::Iterator::Ptr Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound);

  void Compact();

private:
    io::Manager& io_manager_;
    table::Table::Ptr impl_;
};

}

