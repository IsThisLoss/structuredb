#pragma once

#include <io/file_reader.hpp>

namespace structuredb::server::lsm::disk {

class Page {
public:
  static boost::asio::awaitable<Page> Load(io::FileReader& file_reader);

  std::optional<std::string> Find(const std::string& key) const;
private:
  std::vector<std::string> keys_;
  std::vector<std::string> values_;
};

}
