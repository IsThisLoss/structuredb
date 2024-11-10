#pragma once

#include <fstream>

#include "page_builder.hpp"

namespace structuredb::server::lsm::disk {

class FileBuilder {
public:
  explicit FileBuilder(const std::string& path, const int64_t page_size);

  FileBuilder& Add(const std::string& key, const std::string& value);

  void Finish() &&;
private:
  const int64_t page_size_{0};
  std::ofstream of_;

  int64_t added_{};
  PageBuilder current_page_;
};

}
