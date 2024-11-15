#pragma once

#include <io/file_writer.hpp>

namespace structuredb::server::sdb {

/// @brief helper class to write into .sdb files
class Writer {
public:
  explicit Writer(io::FileWriter::Ptr file_writer);

  Awaitable<void> Rewind();

  Awaitable<void> FSync();

  /// @brief reads string from reader provided in constructor
  Awaitable<void> WriteString(const std::string& value);

  /// @brief reads int from reader provided in constructor
  Awaitable<void> WriteInt(int64_t value);

  /// @brief returns size of value after serialization
  static size_t EstimateSize(int64_t value);

  /// @brief returns size of value after serialization
  static size_t EstimateSize(const std::string& value);
private:
  io::FileWriter::Ptr file_writer_;
};

}

