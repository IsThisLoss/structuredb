#pragma once

#include <memory>

#include <io/types.hpp>

namespace structuredb::server::table {

class Table {
public:
  using Ptr = std::shared_ptr<Table>;

  virtual Awaitable<void> Upsert(
      const std::string& key,
      const std::string& value
  ) = 0;

  virtual Awaitable<std::optional<std::string>> Lookup(const std::string& key) = 0;

  virtual Awaitable<bool> Delete(const std::string& key) = 0;

  virtual Awaitable<std::vector<std::pair<std::string, std::string>>> GetAll() = 0;

  virtual ~Table() = default;
};

}
