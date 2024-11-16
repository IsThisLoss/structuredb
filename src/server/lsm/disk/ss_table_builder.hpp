#pragma once

#include <io/file_writer.hpp>

#include "ss_table_header.hpp"
#include "page_builder.hpp"

namespace structuredb::server::lsm::disk {

class SSTableBuilder {
public:
  static Awaitable<SSTableBuilder> Create(io::FileWriter::Ptr file_writer, const int64_t page_size);

  explicit SSTableBuilder(io::FileWriter::Ptr file_writer, const int64_t page_size);

  Awaitable<void> Init();

  Awaitable<void> Add(const std::string& key, const std::string& value);

  Awaitable<void> Finish() &&;
private:
  bool is_initialized_{false};

  SSTableHeader header_;

  io::FileWriter::Ptr file_writer_;

  PageBuilder page_builder_;

  Awaitable<void> FlushHeader();

  Awaitable<void> FlushPage();
};

}
