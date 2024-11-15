#pragma once

#include <optional>

#include <sdb/reader.hpp>

namespace structuredb::server::lsm::disk {

class Page {
public:
  static Awaitable<Page> Load(sdb::Reader& reader);

  /// @brief searches key in the page
  ///
  /// Time complexity is O(n)
  std::optional<std::string> Find(const std::string& key) const;

  /// @returns minimum key in the page
  const std::string& MinKey() const;

  /// @returns maximum key in the page
  const std::string& MaxKey() const;
private:
  /// @property keys places inside the page
  std::vector<std::string> keys_;

  /// @property values places inside the page
  std::vector<std::string> values_;
};

}
