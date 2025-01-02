#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include "types.hpp"
#include "blocking_executor.hpp"

namespace structuredb::server::io {

class FileReader {
public:
  using Ptr = std::shared_ptr<FileReader>;

  explicit FileReader(
      boost::asio::io_context& io_context,
      BlockingExecutor& blocking_executor
  );
  
  Awaitable<void> Open(std::string path);

  Awaitable<size_t> Read(char* buffer, size_t size);

  Awaitable<void> Seek(size_t pos);

  const std::string& Path();

  ~FileReader();
private:
  std::string path_;
  boost::asio::posix::stream_descriptor stream_; 
  BlockingExecutor& blocking_executor_;
};

}
