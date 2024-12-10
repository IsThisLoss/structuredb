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

Awaitable<std::vector<std::pair<std::string, std::string>>> SystemView::GetAll() {
  throw DatabaseException{"Unimplemented"};
}

}
