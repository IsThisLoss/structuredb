#include "manager.hpp"

namespace structuredb::server::io {

Manager::Manager(boost::asio::io_context& io_context)
  : io_context_{io_context}
{}

FileReader::Ptr Manager::CreateFileReader(const std::string& path) const {
  return std::make_shared<FileReader>(io_context_, path);
}

FileWriter Manager::CreateFileWriter(const std::string& path) const {
  return FileWriter{io_context_, path};
}

}
