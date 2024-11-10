#pragma once

#include <fstream>
#include <string>

namespace structuredb::server::lsm {

class SSTable {
public:
  explicit SSTable(const std::string& file_path);

  std::optional<std::string> Get(const std::string& key) const;

  ~SSTable();
private:
  // |----------|
  // |  HEADER  |
  // | min:     |
  // | max:     |
  // |----------|
  // |  PAGES   |
  // | record   |
  // page
  
  struct Header {
    size_t pages_count;
  };

  std::fstream fin;
};

}
