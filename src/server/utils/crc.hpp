#pragma once

#include <string>

#include <boost/crc.hpp>

namespace structuredb::server::utils {

class Crc {
public:
  void Update(const std::string& str);

  void Update(const int64_t value);

  void Clear();

  int32_t Result();
private:
  boost::crc_32_type impl_{};
};

}
