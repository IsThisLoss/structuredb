#include "write_buffer.hpp"

namespace structuredb::server::lsm::disk {

size_t WriteBuffer::CalculateRecordSize(const std::string& key, const std::string& value) {
  return sizeof(int64_t) + key.size() + sizeof(int64_t) + value.size();
}

WriteBuffer::WriteBuffer(size_t size)
  : impl_(size)
  , pos_{impl_.data()}
{
}

void WriteBuffer::Rewind() {
  pos_ = impl_.data();
}

void WriteBuffer::WriteInt(const int64_t value) {
  Write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void WriteBuffer::WriteString(const std::string value) {
  WriteInt(value.size());
  Write(value.data(), value.size());
}

size_t WriteBuffer::Size() const {
  return pos_ - impl_.data();
}

Awaitable<void> WriteBuffer::Flush(io::FileWriter& file_writer) {
  co_await file_writer.Write(impl_.data(), impl_.size());
  impl_.clear();
}

void WriteBuffer::Write(const char* data, size_t size) {
  ::memcpy(pos_, data, size);
  pos_ += size;
}

}
