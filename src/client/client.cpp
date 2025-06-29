#include "client.hpp"

#include <connection/connection.hpp>

namespace structuredb::client {

database::AutoCommitDatabase::Ptr Connect(const std::string& addr) {
  auto connection = connection::MakeConnection(addr);
  return database::MakeAutoCommitDatabase(std::move(connection));
}

}

