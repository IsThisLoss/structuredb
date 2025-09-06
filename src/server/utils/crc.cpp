#include "crc.hpp"

namespace structuredb::server::utils {

void Crc::Update(const void* data, size_t size) {
  impl_.process_bytes(data, size);
}

void Crc::Clear() {
  impl_.reset();
}

int32_t Crc::Result() {
  return static_cast<int32_t>(impl_.checksum());
}

}
