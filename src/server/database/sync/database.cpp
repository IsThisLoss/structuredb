#include "database.hpp"

namespace structuredb::server::database::sync {

Database::Database(io::Manager& io_manager, database::Database&& impl)
  : io_manager_{io_manager}
  , impl_{std::move(impl)}
{
}

void Database::Init() {
  io_manager_.RunSync(impl_.Init());
}

table::storage::Storage::Ptr Database::GetStorageForRecover(const table::storage::Storage::Id& storage_id) {
  return impl_.GetStorageForRecover(storage_id);
}

sync::Session Database::StartSession(const std::optional<transaction::TransactionId>& tx) {
  auto async_session = io_manager_.RunSync(
      [this, tx]() -> Awaitable<database::Session::Ptr> {
      auto session = co_await impl_.StartSession(tx);
      co_return std::make_shared<database::Session>(std::move(session));
  }());
  return sync::Session{io_manager_, std::move(async_session)};
}

}
