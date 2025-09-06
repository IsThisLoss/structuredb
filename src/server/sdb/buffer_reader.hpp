#pragma once

#include "reader.hpp"

#include <vector>

namespace structuredb::server::sdb {

/// @brief implementation of Reader that reads from a buffer
class BufferReader : public Reader {
public:
  explicit BufferReader(std::vector<char> data);

  std::string ReadString() override;

  int64_t ReadInt() override;

  bool HasMore() const;
private:
  std::vector<char> data_;
  const char* buf_;
};

}
