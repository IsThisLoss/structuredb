#pragma once

#include <database/session.hpp>
#include <table/sync/table.hpp>

namespace structuredb::server::database::sync {

class Session {
public:
 explicit Session(
    io::Manager& io_manager,
    database::Session::Ptr impl
 );

 void Start(const std::optional<transaction::TransactionId>& tx = std::nullopt);

 transaction::TransactionId Finish();

 transaction::TransactionId GetTx();

 void CreateTable(const std::string& name);

 void DropTable(const std::string& name);

 sync::Session Begin();

 void Commit();

 table::sync::Table::Ptr GetTable(const std::string& name);

 void CompactTable(const std::string& name);

 int CountSSTables(const std::string& name);
private:
  io::Manager& io_manager_;
  database::Session::Ptr impl_;
};
}
