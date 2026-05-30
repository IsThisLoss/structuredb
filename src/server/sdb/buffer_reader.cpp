#include <string>
#include <vector>

#include "buffer_reader.hpp"

namespace structuredb::server::sdb {

BufferReader::BufferReader(std::vector<char> data)
  : data_{std::move(data)}
  , buf_{data_.data()}
{}

void BufferReader::Read(char* buffer, size_t size) {
  ::memcpy(buffer, buf_, size);
  buf_ += size;
}

std::string BufferReader::ReadString() {
  const int64_t size = ReadInt();
  std::string result;
  result.resize(size);
  Read(result.data(), size);
  return result;
}

int64_t BufferReader::ReadInt() {
  int64_t value{0};
  Read(reinterpret_cast<char*>(&value), sizeof(int64_t));
  return value;
}

bool BufferReader::ReadBool() {
  bool value = false;
  Read(reinterpret_cast<char*>(&value), sizeof(bool));
  return value;
}

bool BufferReader::HasMore() const {
  return buf_ < data_.data() + data_.size();
}

}
