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
};

}
