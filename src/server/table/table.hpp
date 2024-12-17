#pragma once

#include <memory>

#include <io/types.hpp>

namespace structuredb::server::table {

/// @brief table interface
class Table {
public:
  using Ptr = std::shared_ptr<Table>;

  /// @brief inserts or updates value by key
  virtual Awaitable<void> Upsert(
      const std::string& key,
      const std::string& value
  ) = 0;

  /// @brief returns value assossiated with key in score of current transaction
  virtual Awaitable<std::optional<std::string>> Lookup(const std::string& key) = 0;

  /// @brief deletes key from table
  virtual Awaitable<bool> Delete(const std::string& key) = 0;

  /// @brief returns all key, values from given range
  virtual Awaitable<std::vector<std::pair<std::string, std::string>>> Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound) = 0;

  virtual Awaitable<void> Compact() { co_return; };

  virtual ~Table() = default;
};

}
