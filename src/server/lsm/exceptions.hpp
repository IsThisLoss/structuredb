#pragma once

#include <stdexcept>

namespace structuredb::server::lsm {

class CurraptedSSTable : public std::runtime_error{
public:
  using std::runtime_error::runtime_error;
};

}

