#pragma once

#include <io/manager.hpp>
#include <database/database.hpp>

namespace structuredb::server::wal {

Awaitable<void> Clean(io::Manager& io_manager, const std::string& wal_dir_path, database::Database& db);

}
