#include "file_reader.hpp"

#include <boost/asio/use_awaitable.hpp>

#include <sys/file.h>

namespace structuredb::server::io {

FileReader::FileReader(boost::asio::io_context& io_context, const std::string& path) 
  :
  io_context_{io_context},
  stream_{io_context_, ::open(path.c_str(), O_RDONLY)}
{}

boost::asio::awaitable<size_t> FileReader::Read(char* buffer, size_t size) {
  const size_t result = co_await stream_.async_read_some(
      boost::asio::buffer(buffer, size),
      boost::asio::use_awaitable
  );
  co_return result;
}

FileReader::~FileReader() {
  try {
    stream_.close();
  } catch (const std::exception& e) {
    // TODO
  }
}

}

