#pragma once

#include <lsm/types.hpp>
#include <sdb/writer.hpp>

#include <utils/crc.hpp>

namespace structuredb::server::lsm::disk {

class PageBuilder {
public:
  explicit PageBuilder(const int64_t max_bytes_size);

  void Clear();

  bool IsEnoughPlace(const Record& record) const;

  void Add(const Record& record);

  bool IsEmpty() const;

  Awaitable<void> Flush(io::FileWriter& writer);
private:
  const int64_t max_bytes_size_;

  size_t current_size_;
  utils::Crc crc_;

  /// @property keys places inside the page
  std::vector<std::string> keys_;

  std::vector<Sequence> seq_nos_;

  /// @property values places inside the page
  std::vector<std::string> values_;
};

}
