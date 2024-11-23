#include "file_reader.hpp"

#include <spdlog/spdlog.h>

#include <sys/file.h>

#include "exceptions.hpp"

namespace structuredb::server::io {

FileReader::FileReader(
    boost::asio::io_context& io_context,
    BlockingExecutor& blocking_executor
)  : stream_{io_context}
   , blocking_executor_{blocking_executor}
{}

Awaitable<void> FileReader::Open(std::string path) {
  int fd = co_await blocking_executor_.Execute([&] () -> int {
    return ::open(path.c_str(), O_RDONLY);
  });
  stream_.assign(fd);
  spdlog::info("Open file {} for read, fd = {}", path, fd);
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
    spdlog::error("Unable to read: {}", strerror(errno));
    throw;
  }
  co_return 0;
}

Awaitable<void> FileReader::Seek(size_t pos) {
  co_await blocking_executor_.Execute([&]() {
    ::lseek(stream_.native_handle(), pos, SEEK_SET);
  });
}

FileReader::~FileReader() {
  try {
    stream_.close();
  } catch (const std::exception& e) {
    spdlog::error("Unable to read: {}, {}", e.what(), strerror(errno));
  }
}

}

