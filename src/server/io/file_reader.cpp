#include "file_reader.hpp"

#include <iostream>

#include <boost/asio/use_awaitable.hpp>

#include <sys/file.h>

#include "exceptions.hpp"

namespace structuredb::server::io {

FileReader::FileReader(boost::asio::io_context& io_context, const std::string& path) 
  :
  io_context_{io_context},
  stream_{io_context_}
{
  int fd = ::open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    perror("Failed to open file");
  }
  stream_.assign(fd);
  auto size = ::lseek(fd, 0, SEEK_END);
  ::lseek(fd, 0, SEEK_SET);
  std::cerr << "Opened " << path << " for read, size: " << size << "\n";
}

Awaitable<size_t> FileReader::Read(char* buffer, size_t size) {
  try {
    const size_t result = co_await stream_.async_read_some(
        boost::asio::buffer(buffer, size),
        boost::asio::use_awaitable
    );
    co_return result;
  } catch (const boost::system::system_error& e) {
    if (e.code() == boost::asio::error::eof) {
      throw EndOfFile{e.what()};
    }
    throw;
  } catch (const std::exception& e) {
    perror("Unable to read");
    std::cerr << "Read: " << typeid(e).name() << e.what();
    throw;
  }
  co_return 0;
}

Awaitable<void> FileReader::Seek(size_t pos) {
  auto off = ::lseek(stream_.native_handle(), pos, SEEK_SET);
  std::cerr << "lseek: " << off << std::endl;
  co_return;
}

FileReader::~FileReader() {
  try {
    stream_.close();
  } catch (const std::exception& e) {
    // TODO
  }
}

}

