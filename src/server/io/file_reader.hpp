#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/awaitable.hpp>

#include "types.hpp"

namespace structuredb::server::io {

class FileReader {
public:
  explicit FileReader(boost::asio::io_context& io_context, const std::string& path);

  FileReader(FileReader&& other);

  Awaitable<size_t> Read(char* buffer, size_t size);

  Awaitable<void> Seek(size_t pos);

  ~FileReader();
private:
  boost::asio::io_context& io_context_;
  boost::asio::posix::stream_descriptor stream_; 
};

}
