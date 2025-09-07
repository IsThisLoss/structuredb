#pragma once

#include <string>

namespace structuredb::server::sdb {

/// @brief interface of sdb data format reader
class Reader {
public:
  /// @brief reads binary data
  virtual void Read(char* buffer, size_t size) = 0;

  /// @brief reads string from reader
  virtual std::string ReadString() = 0;

  /// @brief reads int from reader
  virtual int64_t ReadInt() = 0;

  /// @brief reads bool from reader
  virtual bool ReadBool() = 0;

  virtual ~Reader() = default;
};

}
