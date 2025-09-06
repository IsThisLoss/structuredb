#pragma once

#include <vector>

namespace structuredb::server::lsm::disk {

int64_t GetPageChecksum(const std::vector<char>& page_buffer);

}
