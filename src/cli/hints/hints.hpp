#pragma once

#include <string>
#include <unordered_map>

namespace structuredb::cli {

class Hints {
public:
  Hints& Add(const std::string& key, const std::string& value);

  std::string& GetHint(const std::string& key);

private:
  std::unordered_map<std::string, std::string> hints_;
};

}
