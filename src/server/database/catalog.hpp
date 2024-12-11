#pragma once

#include <unordered_set>
#include <table/transactional_table.hpp>

namespace structuredb::server::database {

/// @brief catalog of tables
///
/// Store mapping from table name to table id (folder on disk)
/// in score of transaction
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

  /// @brief returns reserved table names that cannot by used by database user
  static const std::unordered_set<std::string>& GetInternalTableNames();

  /// @brief adds table name to catalog
  ///
  /// @return table id that should be used as folder name on disk
  Awaitable<std::string> AddStorage(const std::string& name);

  /// @brief removes table name from catalog
  Awaitable<void> DeleteStorage(const std::string& name);

  /// @brief returns table id by its name
  Awaitable<std::optional<std::string>> GetStorageId(const std::string& name);
private:
  table::Table::Ptr sys_tables_;

  Awaitable<std::optional<TableInfo>> GetTableInfo(const std::string& name);
};

}
