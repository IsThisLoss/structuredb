#pragma once

#include <boost/crc.hpp>

namespace structuredb::server::utils {

class Crc {
public:
  void Update(const void* data, size_t size);

  void Clear();

  int32_t Result();
private:
  boost::crc_32_type impl_{};
};

}
