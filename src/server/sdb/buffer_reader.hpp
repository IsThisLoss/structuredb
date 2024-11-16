#pragma once

#include <io/file_reader.hpp>

namespace structuredb::server::sdb {

class BufferReader {
public:
  explicit BufferReader(std::vector<char> data);

  Awaitable<std::string> ReadString();

  Awaitable<int64_t> ReadInt();
private:
  std::vector<char> data_;
  const char* buf_;
};

}
