#include "reader.hpp"

namespace structuredb::server::sdb {

Reader::Reader(io::FileReader& file_reader)
  : file_reader_{file_reader}
{}

Awaitable<std::string> Reader::ReadString() {
  const int64_t size = co_await ReadInt();
  std::string result;
  result.resize(size);
  co_await file_reader_.Read(result.data(), size);
  co_return result;
}

Awaitable<int64_t> Reader::ReadInt() {
  int64_t value{0};
  co_await file_reader_.Read(reinterpret_cast<char*>(&value), sizeof(int64_t));
  co_return value;
}

}
