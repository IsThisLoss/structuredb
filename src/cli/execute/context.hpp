#pragma once

#include <database/autocommit_database.hpp>

namespace structuredb::cli {

struct Context {
  client::database::AutoCommitDatabase::Ptr db;
  client::database::Transaction::Ptr tx;
};

}
