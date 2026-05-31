#pragma once

#include "writer.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace structuredb::server::sdb {

/// @brief implementation of Writer that writes to a buffer
class BufferWriter : public Writer {
public:

  void Write(const char* buffer, size_t size) override;

  void WriteString(const std::string& value) override;

  void WriteInt(int64_t value) override;

  void WriteBool(bool value) override;

  std::vector<char> Extract() &&;

private:
  std::vector<char> data_;
};

}


