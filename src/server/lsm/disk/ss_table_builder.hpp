#pragma once

#include <io/file_writer.hpp>

#include "write_buffer.hpp"
#include "ss_table_header.hpp"

namespace structuredb::server::lsm::disk {

class SSTableBuilder {
public:
  explicit SSTableBuilder(io::FileWriter& file_writer, const int64_t page_size);

  Awaitable<void> Init();

  Awaitable<void> Add(const std::string& key, const std::string& value);

  Awaitable<void> Finish() &&;
private:
  bool is_initialized_{false};

  SSTableHeader header_;

  io::FileWriter& file_writer_;

  int64_t current_page_size_;
  WriteBuffer write_buffer_;

  void InitBuffer();

  Awaitable<void> FlushBuffer();

  Awaitable<void> WriteHeader();
};

}
