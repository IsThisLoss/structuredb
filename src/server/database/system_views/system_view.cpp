#include "system_view.hpp"

#include <database/exceptions.hpp>

namespace structuredb::server::database::system_views {

Awaitable<void> SystemView::Upsert(
    const std::string& key,
    const std::string& value
) {
  throw DatabaseException{"Cannot perform mutation on system view"};
}

Awaitable<bool> SystemView::Delete(const std::string& key) {
  throw DatabaseException{"Cannot perform mutation on system view"};
}

Awaitable<table::Iterator::Ptr> SystemView::Scan(const std::optional<std::string>& lower_bound, const std::optional<std::string>& upper_bound) {
  throw DatabaseException{"Unimplemented"};
}

}
