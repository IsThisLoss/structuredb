#pragma once

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>

#include "file_reader.hpp"
#include "file_writer.hpp"
#include "blocking_executor.hpp"

namespace structuredb::server::io {

class Manager {
public:
  explicit Manager(boost::asio::io_context& io_context);

  Awaitable<FileReader::Ptr> CreateFileReader(const std::string& path);

  Awaitable<FileWriter::Ptr> CreateFileWriter(const std::string& path, bool append = false);

  Awaitable<void> CreateDirectory(const std::string& path);

  Awaitable<std::vector<std::string>> ListDirectory(const std::string& path);

  Awaitable<void> Remove(const std::string& path);

  template <std::invocable<> Coro>
  void CoSpawn(Coro&& coro) const {
    boost::asio::co_spawn(io_context_, std::forward<Coro>(coro), boost::asio::detached);
  }
private:
  boost::asio::io_context& io_context_;
  BlockingExecutor blocking_executor_;
};

}
