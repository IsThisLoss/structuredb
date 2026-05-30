#include <string>

#include "segment.hpp"

namespace structuredb::server::wal {

int64_t GetSegmentNoFromName(const std::string& name) {
  try {
    return std::stoll(name);
  } catch (const std::exception& ex) {
    return -1;
  }
}

}
