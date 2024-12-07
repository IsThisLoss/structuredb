#include "sys_transactions.hpp"

namespace structuredb::server::database::system_views {

SysTransactions::SysTransactions(transaction::Storage::Ptr tx_storage)
  : tx_storage_{std::move(tx_storage)}
{}

Awaitable<std::optional<std::string>> SysTransactions::Lookup(const std::string& key) {
  co_return co_await tx_storage_->GetStatus(transaction::FromString(key));
}

}
