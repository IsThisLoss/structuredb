#pragma once

#include <io/file_reader.hpp>

namespace structuredb::server::lsm::disk {

class Page {
public:
  static Awaitable<Page> Load(io::FileReader& file_reader);

  std::optional<std::string> Find(const std::string& key) const;

  const std::string& MinKey() const;

  const std::string& MaxKey() const;
private:
  int64_t size_;
  std::vector<std::string> keys_;
  std::vector<std::string> values_;
};

}
