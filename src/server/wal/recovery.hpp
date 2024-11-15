#pragma once

#include <io/manager.hpp>
#include <lsm/lsm.hpp>

namespace structuredb::server::wal {

Awaitable<void> Recover(io::Manager& io_manager, const std::string& path, lsm::Lsm& lsm);

}
