#include "manager.hpp"

#include <iostream>

namespace structuredb::server::io {

namespace {

constexpr int kThreadNum = 2;
}

Manager::Manager(boost::asio::io_context& io_context)
  : io_context_{io_context}
  , blocking_executor_{kThreadNum}
{}

Awaitable<FileReader::Ptr> Manager::CreateFileReader(const std::string& path) {
  auto result = std::make_shared<FileReader>(io_context_, blocking_executor_);
  co_await result->Open(path);
  co_return result;
}

Awaitable<FileWriter::Ptr> Manager::CreateFileWriter(const std::string& path, bool append) {
  auto result = std::make_shared<FileWriter>(io_context_, blocking_executor_);
  co_await result->Open(path, append);
  co_return result;
}

}
