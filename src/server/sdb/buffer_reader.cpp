#include "buffer_reader.hpp"

namespace structuredb::server::sdb {

BufferReader::BufferReader(std::vector<char> data)
  : data_{data}
  , buf_{data_.data()}
{}

Awaitable<std::string> BufferReader::ReadString() {
  const int64_t size = co_await ReadInt();
  std::string result;
  result.resize(size);
  ::memcpy(result.data(), buf_, size);
  buf_ += size;
  co_return result;
}

Awaitable<int64_t> BufferReader::ReadInt() {
  int64_t value{0};
  value = *reinterpret_cast<const int64_t*>(buf_);
  buf_ += sizeof(int64_t);
  co_return value;
}

}
