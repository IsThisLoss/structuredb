#pragma once

#include <stdexcept>

namespace structuredb::server::io {

class EndOfFile : public std::runtime_error{
public:
  using std::runtime_error::runtime_error;
};

}
