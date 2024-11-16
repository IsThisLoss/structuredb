#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/awaitable.hpp>

#include "types.hpp"

namespace structuredb::server::io {

class FileWriter {
public:
  using Ptr = std::shared_ptr<FileWriter>;

  explicit FileWriter(boost::asio::io_context& io_context, const std::string& path, const bool append);

  Awaitable<size_t> Write(const char* buffer, size_t size);

  Awaitable<void> Rewind();

  Awaitable<void> FSync();

  ~FileWriter();
private:
  boost::asio::io_context& io_context_;
  boost::asio::posix::stream_descriptor stream_; 
};

}
