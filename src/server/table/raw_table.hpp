#pragma once

#include "table.hpp"

#include <transaction/storage.hpp>
#include <table/storage/storage.hpp>

namespace structuredb::server::table {

/// @brief implements table interface with direct access to storage
class RawTable : public Table {
public:
  explicit RawTable(storage::Storage::Ptr table_storage);

  Awaitable<void> Upsert(
      const std::string& key,
      const std::string& value
  ) override;

  Awaitable<std::optional<std::string>> Lookup(const std::string& key) override;

  Awaitable<bool> Delete(const std::string& key) override;

  Awaitable<Iterator::Ptr> Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound) override;

  Awaitable<void> Compact() override;
private:
  storage::Storage::Ptr table_storage_;
};

}

