#pragma once

#include <database/catalog.hpp>

#include "system_view.hpp"

namespace structuredb::server::database::system_views {

/// @brief system view for tables
class SysTables : public SystemView {
public:
  explicit SysTables(Catalog catalog);

  Awaitable<std::optional<std::string>> Lookup(const std::string& key) override;

private:
  Catalog catalog_;
};

}
