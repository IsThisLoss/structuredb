#pragma once

#include <string>

namespace structuredb::server::sdb {

/// @brief interface of sdb data format writer
class Writer {
public:
  /// @brief writes string
  virtual void WriteString(const std::string& value) = 0;

  /// @brief writes int64_t
  virtual void WriteInt(int64_t value) = 0;

  virtual ~Writer() = default;
};

}
