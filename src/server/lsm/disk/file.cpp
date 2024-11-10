#include "file.hpp"

#include <fstream>
#include <cassert>

namespace structuredb::server::lsm::disk {

File File::Load(const std::string& path) {
  File result{};

  std::ifstream ifs;
  ifs.open(path, std::ios::binary);
  assert(ifs.is_open());

  while (!ifs.eof()) {
    auto page = Page::Load(ifs);
    result.pages_.push_back(std::move(page));
  }

  return result;
}

std::optional<std::string> File::Find(const std::string& key) const {
  // FIXME use binary search
  for (const auto& page : pages_) {
    const auto value = page.Find(key);
    if (value.has_value()) {
      return value;
    }
  }
  return std::nullopt;
}

}
