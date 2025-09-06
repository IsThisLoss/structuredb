#include "buffer_writer.hpp"

namespace structuredb::server::sdb {

void BufferWriter::WriteString(const std::string& value) {
  WriteInt(static_cast<int64_t>(value.size()));
  data_.reserve(data_.size() + value.size());
  data_.insert(data_.end(), value.begin(), value.end());
}

void BufferWriter::WriteInt(int64_t value) {
  const char* bytes = reinterpret_cast<const char*>(&value);
  data_.insert(data_.end(), bytes, bytes + sizeof(int64_t));
}

std::vector<char> BufferWriter::Extract() && {
  auto result = std::move(data_);
  data_.clear();
  return result;
}

}

