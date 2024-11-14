#pragma once

#include <sys/types.h>

namespace structuredb::server::lsm::disk {

struct SSTableHeader {
  int64_t page_size{0};
  int64_t page_count{0};
};

}
