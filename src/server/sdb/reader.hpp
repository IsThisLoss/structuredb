#pragma once

#include <io/file_reader.hpp>

namespace structuredb::server::sdb {

/// @brief helper class to read from .sdb files
class Reader {
public:
  explicit Reader(io::FileReader::Ptr file_reader);

  Awaitable<void> Seek(size_t pos);

  /// @brief reads string from reader provided in constructor
  Awaitable<std::string> ReadString();

  /// @brief reads int from reader provided in constructor
  Awaitable<int64_t> ReadInt();
private:
  io::FileReader::Ptr file_reader_;
};

}
