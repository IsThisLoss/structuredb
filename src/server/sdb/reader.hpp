#pragma once

#include <string>

namespace structuredb::server::sdb {

/// @brief interface of sdb data format reader
class Reader {
public:
  /// @brief reads string from reader provided in constructor
  virtual std::string ReadString() = 0;

  /// @brief reads int from reader provided in constructor
  virtual int64_t ReadInt() = 0;

  virtual ~Reader() = default;
};

}
