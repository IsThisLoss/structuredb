#pragma once

#include <sdb/buffer_reader.hpp>

#include <lsm/types.hpp>

namespace structuredb::server::lsm::disk {

class Page {
public:
  static Awaitable<Page> Load(sdb::BufferReader& reader);

  /// @brief searches key in the page
  std::vector<std::string> Find(const std::string& key) const;

  /// @returns minimum key in the page
  const std::string& MinKey() const;

  /// @returns maximum key in the page
  const std::string& MaxKey() const;
private:
  /// @property keys places inside the page
  std::vector<std::string> keys_;

  std::vector<Sequence> seq_nos_;

  /// @property values places inside the page
  std::vector<std::string> values_;
};

}
