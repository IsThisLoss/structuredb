#pragma once

#include "reader.hpp"

#include <string>
#include <vector>

namespace structuredb::server::sdb {

/// @brief implementation of Reader that reads from a buffer
class BufferReader : public Reader {
public:
  explicit BufferReader(std::vector<char> data);

  void Read(char* buffer, size_t size) override;

  std::string ReadString() override;

  int64_t ReadInt() override;

  bool ReadBool() override;

  bool HasMore() const;
private:
  std::vector<char> data_;
  const char* buf_;
};

}
