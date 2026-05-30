#pragma once

#include <string>
#include <database/autocommit_database.hpp>

namespace structuredb::client {

database::AutoCommitDatabase::Ptr Connect(const std::string& addr);

}
