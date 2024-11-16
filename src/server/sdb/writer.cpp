#include "writer.hpp"

#include <iostream>

namespace structuredb::server::sdb {

Writer::Writer(io::FileWriter::Ptr file_writer)
  : file_writer_{std::move(file_writer)}
{}

Awaitable<void> Writer::Rewind() {
  co_await file_writer_->Rewind();
}

Awaitable<void> Writer::FSync() {
  co_await file_writer_->FSync();
}

Awaitable<void> Writer::WriteString(const std::string& value) {
  co_await WriteInt(value.size());
  co_await file_writer_->Write(reinterpret_cast<const char*>(value.data()), value.size());
}

Awaitable<void> Writer::WriteInt(int64_t value) {
  co_await file_writer_->Write(reinterpret_cast<const char*>(&value), sizeof(int64_t));
}

size_t Writer::EstimateSize(int64_t value) {
  return sizeof(int64_t);
}

size_t Writer::EstimateSize(const std::string& value) {
  return sizeof(int64_t) + value.size(); // size + value
}

}