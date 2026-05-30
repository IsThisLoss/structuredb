#include <cstdint>
#include <string>
#include <vector>

#include "buffer_writer.hpp"

namespace structuredb::server::sdb {

void BufferWriter::Write(const char* buffer, size_t size) {
  data_.reserve(data_.size() + size);
  data_.insert(data_.end(), buffer, buffer + size);
}

void BufferWriter::WriteString(const std::string& value) {
  WriteInt(static_cast<int64_t>(value.size()));
  Write(value.data(), value.size());
}

void BufferWriter::WriteInt(int64_t value) {
  const char* bytes = reinterpret_cast<const char*>(&value);
  data_.insert(data_.end(), bytes, bytes + sizeof(int64_t));
}

void BufferWriter::WriteBool(bool value) {
  const char* bytes = reinterpret_cast<const char*>(&value);
  data_.insert(data_.end(), bytes, bytes + sizeof(bool));
}

std::vector<char> BufferWriter::Extract() && {
  auto result = std::move(data_);
  data_.clear();
  return result;
}

}
