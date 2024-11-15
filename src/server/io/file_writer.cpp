#include "file_writer.hpp"

#include <iostream>

#include <boost/asio/use_awaitable.hpp>

#include <sys/file.h>

namespace structuredb::server::io {

FileWriter::FileWriter(boost::asio::io_context& io_context, const std::string& path) 
  :
  io_context_{io_context},
  stream_{io_context_}
{
  ::unlink(path.c_str());
  int fd = ::open(path.c_str(), O_WRONLY | O_CREAT, S_IRWXU);
  if (fd < 0) {
    perror("Failed to open file");
  }
  stream_.assign(fd);
  std::cerr << "Opened " << path << " " << fd << " for write\n";
}

Awaitable<size_t> FileWriter::Write(const char* buffer, size_t size) {
  try {
  const size_t result = co_await stream_.async_write_some(
      boost::asio::buffer(buffer, size),
      boost::asio::use_awaitable
  );
  co_return result;
  } catch (const std::exception& e) {
    std::cerr << "Write: " << e.what();
    throw;
  }
}

Awaitable<void> FileWriter::Rewind() {
  ::lseek(stream_.native_handle(), 0, SEEK_SET);
  co_return;
}

Awaitable<void> FileWriter::FSync() {
  ::fsync(stream_.native_handle());
  co_return;
}

FileWriter::~FileWriter() {
  try {
    stream_.close();
    std::cerr << "FileWriter closed\n";
  } catch (const std::exception& e) {
    // TODO
  }
}

}
