#include "writer.hpp"

#include <iostream>

namespace structuredb::server::sdb {

Writer::Writer(io::FileWriter& file_writer)
  : file_writer_{file_writer}
{}

Awaitable<void> Writer::WriteString(const std::string& value) {
  co_await WriteInt(value.size());
  co_await file_writer_.Write(reinterpret_cast<const char*>(value.data()), value.size());
}

Awaitable<void> Writer::WriteInt(int64_t value) {
  std::cerr << "Write int: " << value << std::endl;
  co_await file_writer_.Write(reinterpret_cast<const char*>(&value), sizeof(int64_t));
}

size_t Writer::EstimateSize(int64_t value) {
  return sizeof(int64_t);
}

size_t Writer::EstimateSize(const std::string& value) {
  return sizeof(int64_t) + value.size(); // size + value
}

}
