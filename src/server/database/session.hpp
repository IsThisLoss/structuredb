#pragma once

#include "catalog.hpp"
#include "context.hpp"

namespace structuredb::server::database {

/// @brief database's session
class Session {
public:
  using Ptr = std::shared_ptr<Session>;

  explicit Session(
      Context& context
  );

  /// @brief starts session
  ///
  /// Do not call it directly, use Database::StartSession
 Awaitable<void> Start(const std::optional<transaction::TransactionId>& tx = std::nullopt);

  /// @brief finish session
  ///
  /// @returns transaction id used by session
  ///
  /// if @p tx was not provided at start of session,
  /// implicit transaction created for session
  /// will be commited here
  /// 
  /// NOTE: implicit transaction will not be commited in session's descructor
  /// so be careful to call Finish()
 Awaitable<transaction::TransactionId> Finish();

  /// @returns transaction id used by session
 transaction::TransactionId GetTx();

  /// @brief creates table with given @p name
 Awaitable<void> CreateTable(const std::string& name);

  /// @brief deletes table with given @p name
 Awaitable<void> DropTable(const std::string& name);

 Awaitable<Session> Begin();

 Awaitable<void> Commit();

  /// @brief returns table by its @p name
 Awaitable<table::Table::Ptr> GetTable(const std::string& name);

 /// @brief compacts tables data
 Awaitable<void> CompactTable(const std::string& name);

 /// @brief return the number of ss tables to table
 ///
 /// TODO make more general statistics method
 Awaitable<int> CountSSTables(const std::string& name);

private:
 Context& context_;
 transaction::TransactionId tx_{};
 bool is_autocommit_{true};

 Catalog GetCatalog() const;
};

}
