#include "buffer_reader.hpp"

#include <spdlog/spdlog.h>

namespace structuredb::server::sdb {

BufferReader::BufferReader(std::vector<char> data)
  : data_{std::move(data)}
  , buf_{data_.data()}
{}

std::string BufferReader::ReadString() {
  const int64_t size = ReadInt();
  std::string result;
  result.resize(size);
  ::memcpy(result.data(), buf_, size);
  buf_ += size;
  return result;
}

int64_t BufferReader::ReadInt() {
  int64_t value{0};
  value = *reinterpret_cast<const int64_t*>(buf_);
  buf_ += sizeof(int64_t);
  return value;
}

bool BufferReader::HasMore() const {
  return buf_ < data_.data() + data_.size();
}

}
