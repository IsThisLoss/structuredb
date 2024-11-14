#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/awaitable.hpp>

#include "types.hpp"

namespace structuredb::server::io {

class FileWriter {
public:
  explicit FileWriter(boost::asio::io_context& io_context, const std::string& path);

  Awaitable<size_t> Write(char* buffer, size_t size);

  Awaitable<void> Rewind();

  ~FileWriter();
private:
  boost::asio::io_context& io_context_;
  boost::asio::posix::stream_descriptor stream_; 
};

}
