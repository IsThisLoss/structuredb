#pragma once

#include <table/table.hpp>

namespace structuredb::server::database {

class Catalog {
public:
  enum class TableStatus : int64_t {
    kCreated = 1,
    kDropped = 2,
  };

  struct TableInfo {
    TableStatus status;
    std::string id;
  };

  explicit Catalog(
      table::Table::Ptr table
  );

  Awaitable<std::string> AddTable(const std::string& name);

  Awaitable<void> DeleteTable(const std::string& name);

  Awaitable<std::optional<std::string>> GetTableId(const std::string& name);
private:
  table::Table::Ptr table_;

  Awaitable<std::optional<TableInfo>> GetTableInfo(const std::string& name);
};

}