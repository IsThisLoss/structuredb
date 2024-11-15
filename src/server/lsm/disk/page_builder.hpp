#pragma once

#include <sdb/writer.hpp>

namespace structuredb::server::lsm::disk {

class PageBuilder {
public:
  explicit PageBuilder(const int64_t max_bytes_size);

  void Clear();

  bool IsEnoughPlace(const std::string& key, const std::string& value) const;

  void Add(const std::string& key, const std::string& value);

  bool IsEmpty() const;

  Awaitable<void> Flush(sdb::Writer& writer);
private:
  const int64_t max_bytes_size_;

  size_t current_size_;

  /// @property keys places inside the page
  std::vector<std::string> keys_;

  /// @property values places inside the page
  std::vector<std::string> values_;
};

}
