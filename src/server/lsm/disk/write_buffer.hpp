#pragma once

#include <vector>

#include <io/file_writer.hpp>

namespace structuredb::server::lsm::disk {

class WriteBuffer {
public:
  explicit WriteBuffer(size_t size);

  void Rewind();

  void WriteInt(const int64_t value);

  void WriteString(const std::string value);

  size_t Size() const;

  Awaitable<void> Flush(io::FileWriter& file_writer);

  static size_t CalculateRecordSize(const std::string& key, const std::string& value);
private:
  std::vector<char> impl_{};
  char* pos_{nullptr};

  void Write(const char* data, size_t size);
};

}
