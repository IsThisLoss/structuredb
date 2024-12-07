#pragma once

#include <unordered_set>
#include <table/transactional_table.hpp>

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

  static const std::unordered_set<std::string>& GetInternalTableNames();

  Awaitable<std::string> AddStorage(const std::string& name);

  Awaitable<void> DeleteStorage(const std::string& name);

  Awaitable<std::optional<std::string>> GetStorageId(const std::string& name);
private:
  table::Table::Ptr sys_tables_;

  Awaitable<std::optional<TableInfo>> GetTableInfo(const std::string& name);
};

}
