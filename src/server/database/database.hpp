#pragma once

#include "context.hpp"
#include "session.hpp"

namespace structuredb::server::database {

class Database {
public:
 explicit Database(io::Manager& io_manager, std::string base_dir);

 Awaitable<void> Init();

 table::LsmStorage::Ptr GetStorageForRecover(const table::LsmStorage::Id& storage_id);

  Awaitable<Session> StartSession(const std::optional<transaction::TransactionId>& tx = std::nullopt);

 ~Database() = default;
private:
 Context context_;
 bool is_initialized_{false};
};

}
