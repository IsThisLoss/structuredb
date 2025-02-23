#pragma once

#include <database/database.hpp>

#include "session.hpp"

namespace structuredb::server::database::sync {

/// @brief sync wrapper for database class
class Database {
public:
 explicit Database(io::Manager& io_manager, database::Database&& impl);

 void Init();

 table::storage::Storage::Ptr GetStorageForRecover(const table::storage::Storage::Id& storage_id);

 sync::Session StartSession(const std::optional<transaction::TransactionId>& tx = std::nullopt);

 ~Database() = default;
private:
 io::Manager& io_manager_;
 database::Database impl_;
};

}
