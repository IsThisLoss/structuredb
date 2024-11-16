#pragma once

#include <sys/types.h>
#include <io/manager.hpp>

namespace structuredb::server::sdb {

class BufferWriter {
public:
  explicit BufferWriter(int64_t size);

  Awaitable<void> WriteString(const std::string& value);

  Awaitable<void> WriteInt(int64_t value);

  /// @brief returns size of value after serialization
  static size_t EstimateSize(int64_t value);

  /// @brief returns size of value after serialization
  static size_t EstimateSize(const std::string& value);

  std::vector<char> Extract() &&;
private:
  std::vector<char> data_;
  char* buf_;
};

}


