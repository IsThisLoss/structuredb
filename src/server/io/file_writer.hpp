#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/awaitable.hpp>

#include "types.hpp"
#include "blocking_executor.hpp"

namespace structuredb::server::io {

class FileWriter {
public:
  using Ptr = std::shared_ptr<FileWriter>;

  explicit FileWriter(
      boost::asio::io_context& io_context,
      BlockingExecutor& blocking_executor
  );

  Awaitable<void> Open(std::string path, bool append = false);

  Awaitable<size_t> Write(const char* buffer, size_t size);

  Awaitable<void> Rewind();

  Awaitable<void> FSync();

  ~FileWriter();
private:
  boost::asio::posix::stream_descriptor stream_; 
  BlockingExecutor& blocking_executor_;
};

}
