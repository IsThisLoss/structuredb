#include "tables.hpp"

namespace structuredb::server::database {

namespace {

std::string kCreated = "created";
std::string kDropped = "dropped";

}

Tables::Tables(io::Manager& io_manager, std::string base_dir, table::Table::Ptr table)
  : io_manager_{io_manager}
  , base_dir_{std::move(base_dir)}
  , table_{std::move(table)}
{}

Awaitable<void> Tables::Create(const transaction::TransactionId& tx, const std::string& name) {
  co_await io_manager_.CreateDirectory(base_dir_ + "/" + name);
  co_await table_->Upsert(tx, name, kCreated);
}

Awaitable<void> Tables::Drop(const transaction::TransactionId& tx, const std::string& name) {
  co_await io_manager_.Remove(base_dir_ + "/" + name);
  co_await table_->Upsert(tx, name, kCreated);
}

}
