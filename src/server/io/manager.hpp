#pragma once

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

#include "file_reader.hpp"
#include "file_writer.hpp"

namespace structuredb::server::io {

class Manager {
public:
  explicit Manager(boost::asio::io_context& io_context);

  FileReader::Ptr CreateFileReader(const std::string& path) const;

  FileWriter::Ptr CreateFileWriter(const std::string& path, bool append = false) const;

  template <std::invocable<> Coro>
  void CoSpawn(Coro&& coro) const {
    boost::asio::co_spawn(io_context_, std::forward<Coro>(coro), boost::asio::detached);
  }

  Awaitable<bool> IsFileExists(const std::string& path) const;

private:
  boost::asio::io_context& io_context_;
};

}
