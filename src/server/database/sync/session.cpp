#include "session.hpp"

namespace structuredb::server::database::sync {

Session::Session(
    io::Manager& io_manager,
    database::Session::Ptr impl
) : io_manager_{io_manager}
  , impl_{std::move(impl)}
{
  assert(impl_ != nullptr);
}

void Session::Start(const std::optional<transaction::TransactionId>& tx) {
  io_manager_.RunSync(impl_->Start(tx));
}

transaction::TransactionId Session::Finish() {
  return io_manager_.RunSync(impl_->Finish());
}

transaction::TransactionId Session::GetTx() {
  return impl_->GetTx();
}

void Session::CreateTable(const std::string& name) {
  io_manager_.RunSync(impl_->CreateTable(name));
}

void Session::DropTable(const std::string& name) {
  io_manager_.RunSync(impl_->DropTable(name));
}

sync::Session Session::Begin() {
  auto async_session = io_manager_.RunSync(
      [this]() -> Awaitable<database::Session::Ptr> {
      auto session = co_await impl_->Begin();
      co_return std::make_shared<database::Session>(std::move(session));
  }());
  return sync::Session{io_manager_, std::move(async_session)};
}

void Session::Commit() {
  io_manager_.RunSync(impl_->Commit());
}

table::sync::Table::Ptr Session::GetTable(const std::string& name) {
  auto async_table = io_manager_.RunSync(impl_->GetTable(name));
  if (async_table == nullptr) {
    return nullptr;
  }
  return std::make_shared<table::sync::Table>(io_manager_, std::move(async_table));
}

void Session::CompactTable(const std::string& name) {
  io_manager_.RunSync(impl_->CompactTable(name));
}

int Session::CountSSTables(const std::string& name) {
  return io_manager_.RunSync(impl_->CountSSTables(name));
}

}
