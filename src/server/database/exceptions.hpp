#pragma once

#include <stdexcept>

namespace structuredb::server::database {

class DatabaseException : public std::runtime_error{
public:
  using std::runtime_error::runtime_error;
};

}

