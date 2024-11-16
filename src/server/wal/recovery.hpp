#pragma once

#include <io/manager.hpp>
#include <database/database.hpp>

namespace structuredb::server::wal {

Awaitable<void> Recover(io::Manager& io_manager, const std::string& path, database::Database& db);

}
