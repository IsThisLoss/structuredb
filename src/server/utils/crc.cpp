#include "crc.hpp"

namespace structuredb::server::utils {

void Crc::Update(const std::string& str) {
  impl_.process_bytes(str.data(), str.size());
}

void Crc::Update(const int64_t value) {
  impl_.process_bytes(&value, sizeof(int64_t));
}

void Crc::Clear() {
  impl_.reset();
}

int32_t Crc::Result() {
  return impl_.checksum();
}

}
