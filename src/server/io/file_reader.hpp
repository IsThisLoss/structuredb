#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/awaitable.hpp>

namespace structuredb::server::io {

class FileReader {
public:
  explicit FileReader(boost::asio::io_context& io_context, const std::string& path);

  boost::asio::awaitable<size_t> Read(char* buffer, size_t size);

  ~FileReader();
private:
  boost::asio::io_context& io_context_;
  boost::asio::posix::stream_descriptor stream_; 
};

}
