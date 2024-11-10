#pragma once

#include "page.hpp"

namespace structuredb::server::lsm::disk {

class File {
public:
  static File Load(const std::string& path);
  
  std::optional<std::string> Find(const std::string& key) const;
private:
  std::vector<Page> pages_;
};

}
