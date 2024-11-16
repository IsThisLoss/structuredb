#include "manager.hpp"

#include <iostream>

namespace structuredb::server::io {

Manager::Manager(boost::asio::io_context& io_context)
  : io_context_{io_context}
{}

FileReader::Ptr Manager::CreateFileReader(const std::string& path) const {
  return std::make_shared<FileReader>(io_context_, path);
}

FileWriter::Ptr Manager::CreateFileWriter(const std::string& path, bool append) const {
  return std::make_shared<FileWriter>(io_context_, path, append);
}

Awaitable<bool> Manager::IsFileExists(const std::string& path) const {
  const bool result = access(path.c_str(), F_OK) == 0;
  std::cerr << "IsFileExists " << path << ": " << result << std::endl;
  co_return result;
}

}
