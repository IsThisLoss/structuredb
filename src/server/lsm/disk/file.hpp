#pragma once

#include <boost/asio/awaitable.hpp>

#include "page.hpp"

namespace structuredb::server::lsm::disk {

class File {
public:
  static boost::asio::awaitable<File> Load(boost::asio::io_context& io_context, const std::string& path);
  
  std::optional<std::string> Find(const std::string& key) const;
private:
  std::vector<Page> pages_;
};

}
