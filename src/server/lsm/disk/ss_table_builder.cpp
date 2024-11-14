#include "ss_table_builder.hpp"

#include <iostream>

namespace structuredb::server::lsm::disk {

SSTableBuilder::SSTableBuilder(io::FileWriter& file_writer, const int64_t page_size)
  : file_writer_{file_writer}
  , header_{
    .page_size = page_size,
    .page_count = 1,
  }
  , write_buffer_{static_cast<size_t>(header_.page_size)}
{
}

Awaitable<void> SSTableBuilder::Init() {
  // reserve space for header
  co_await WriteHeader();
  is_initialized_ = true;
}

Awaitable<void> SSTableBuilder::Add(const std::string& key, const std::string& value) {
  assert(is_initialized_);

  int64_t next_record_size = WriteBuffer::CalculateRecordSize(key, value);

  if (write_buffer_.Size() + next_record_size >= header_.page_size) {
    std::cerr << "HERE\n";
    co_await FlushBuffer();
  }

  write_buffer_.WriteString(key);
  write_buffer_.WriteString(value);
  current_page_size_++;
}

Awaitable<void> SSTableBuilder::Finish() && {
  co_await FlushBuffer();

  co_await file_writer_.Rewind();
  co_await WriteHeader();
}

void SSTableBuilder::InitBuffer() {
  // reserve for page_size
  write_buffer_.WriteInt(0);
}

Awaitable<void> SSTableBuilder::FlushBuffer() {
  if (write_buffer_.Size() == sizeof(int64_t)) {
    co_return;
  }
  // write page header
  write_buffer_.Rewind();
  write_buffer_.WriteInt(current_page_size_);
  std::cerr << "Write buffer size: " << current_page_size_ << std::endl;
  current_page_size_ = 0;

  // flush
  co_await write_buffer_.Flush(file_writer_);
  header_.page_count++;

  // init for next page
  InitBuffer();
}

Awaitable<void> SSTableBuilder::WriteHeader() {
  std::cerr << "Start write header: " << sizeof(header_) << std::endl;
  co_await file_writer_.Write(reinterpret_cast<char*>(&header_.page_size), sizeof(int64_t));
  co_await file_writer_.Write(reinterpret_cast<char*>(&header_.page_count), sizeof(int64_t));
  std::cerr << "End write header\n";
}

}
