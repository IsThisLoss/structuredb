#pragma once

#include "writer.hpp"

#include <vector>

namespace structuredb::server::sdb {

/// @brief implementation of Writer that writes to a buffer
class BufferWriter : public Writer {
public:
  void WriteString(const std::string& value) override;

  void WriteInt(int64_t value) override;

  std::vector<char> Extract() &&;

private:
  std::vector<char> data_;
};

}


