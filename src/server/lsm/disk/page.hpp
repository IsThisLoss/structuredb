#pragma once

#include <istream>

namespace structuredb::server::lsm::disk {

class Page {
public:
  static Page Load(std::istream& is);

  std::optional<std::string> Find(const std::string& key) const;
private:
  std::vector<std::string> keys_;
  std::vector<std::string> values_;
};

}
