#pragma once

#include <ostream>

namespace structuredb::server::lsm::disk {

class PageBuilder {
public:
  PageBuilder& Add(const std::string& key, const std::string& value);

  void Serialize(std::ostream& os);

  void Clear();
private:
  std::vector<std::string> keys_;
  std::vector<std::string> values_;
};

}
