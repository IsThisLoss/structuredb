#include "file_writer.hpp"

#include <spdlog/spdlog.h>

#include <boost/asio/use_awaitable.hpp>

#include <sys/file.h>

namespace structuredb::server::io {

FileWriter::FileWriter(
      boost::asio::io_context& io_context,
      BlockingExecutor& blocking_executor
) : stream_{io_context}
  , blocking_executor_{blocking_executor}
{}

Awaitable<void> FileWriter::Open(std::string path, bool append) {
  int fd = co_await blocking_executor_.Execute([&] () -> int {
    int flags = O_WRONLY | O_CREAT;
    if (append) {
      flags |= O_APPEND;
    } else {
      ::unlink(path.c_str());
    }
    return ::open(path.c_str(), flags, S_IRWXU);
  });
  stream_.assign(fd);
  spdlog::info("Open file {} for write, fd = {}", path, fd);
}

Awaitable<size_t> FileWriter::Write(const char* buffer, size_t size) {
  try {
    const size_t result = co_await stream_.async_write_some(
        boost::asio::buffer(buffer, size),
        boost::asio::use_awaitable
    );
    co_return result;
  } catch (const std::exception& e) {
    spdlog::error("Unable to write: {}", strerror(errno));
    throw;
  }
}

Awaitable<void> FileWriter::Rewind() {
  co_await blocking_executor_.Execute([&]() {
    ::lseek(stream_.native_handle(), 0, SEEK_SET);
  });
}

Awaitable<void> FileWriter::FSync() {
  ::fsync(stream_.native_handle());
  co_return;
}

FileWriter::~FileWriter() {
  try {
    stream_.close();
  } catch (const std::exception& e) {
    spdlog::error("Failed to close file: {}, {}", e.what(), strerror(errno));
  }
}

}
