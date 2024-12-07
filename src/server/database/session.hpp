#pragma once

#include "context.hpp"
#include "catalog.hpp"

namespace structuredb::server::database {

class Session {
public:
  explicit Session(
      Context& context
  );

 Awaitable<void> Start(const std::optional<transaction::TransactionId>& tx = std::nullopt);

 Awaitable<transaction::TransactionId> Finish();

 transaction::TransactionId GetTx();

 Awaitable<void> CreateTable(const std::string& name);

 Awaitable<void> DropTable(const std::string& name);

 Awaitable<Session> Begin();

 Awaitable<void> Commit();

 Awaitable<table::Table::Ptr> GetTable(const std::string& name);

private:
 Context& context_;
 transaction::TransactionId tx_{};
 bool is_autocommit_{true};

  Catalog GetCatalog() const;
};

}
