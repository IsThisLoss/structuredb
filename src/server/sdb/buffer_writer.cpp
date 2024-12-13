#include "buffer_writer.hpp"

namespace structuredb::server::sdb {

BufferWriter::BufferWriter(int64_t size)
  : data_(size)
  , buf_{data_.data()}
{}


Awaitable<void> BufferWriter::WriteString(const std::string& value) {
  co_await WriteInt(static_cast<int64_t>(value.size()));
  ::memcpy(buf_, value.data(), value.size());
  buf_ += value.size();
  co_return;
}

Awaitable<void> BufferWriter::WriteInt(int64_t value) {
  ::memcpy(buf_, reinterpret_cast<char*>(&value), sizeof(int64_t));
  buf_ += sizeof(int64_t);
  co_return;
}

int64_t BufferWriter::EstimateSize(int64_t value) {
  return sizeof(int64_t);
}

int64_t BufferWriter::EstimateSize(const std::string& value) {
  return static_cast<int64_t>(sizeof(int64_t)) + static_cast<int64_t>(value.size()); // size + value
}

std::vector<char> BufferWriter::Extract() && {
  auto result = std::move(data_);
  data_.clear();
  buf_ = data_.data();
  return result;
}

}

