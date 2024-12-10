#pragma once

#include <table/table.hpp>

namespace structuredb::server::database::system_views {

class SystemView : public table::Table {
public:
  Awaitable<void> Upsert(
      const std::string& key,
      const std::string& value
  ) override;

  Awaitable<bool> Delete(const std::string& key) override;

  Awaitable<std::vector<std::pair<std::string, std::string>>> Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound) override;
};

}
