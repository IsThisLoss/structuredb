#pragma once

#include <transaction/storage.hpp>

#include "system_view.hpp"

namespace structuredb::server::database::system_views {

class SysTransactions : public SystemView {
public:
  explicit SysTransactions(transaction::Storage::Ptr tx_storage);

  Awaitable<std::optional<std::string>> Lookup(const std::string& key) override;

private:
  transaction::Storage::Ptr tx_storage_;
};

}

