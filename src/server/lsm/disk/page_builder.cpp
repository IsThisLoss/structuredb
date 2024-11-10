#include "page_builder.hpp"

namespace structuredb::server::lsm::disk {

namespace {

void WriteInt(std::ostream& os, const int64_t value) {
  os.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void WriteString(std::ostream& os, const std::string& str) {
  WriteInt(os, str.size());
  os.write(str.data(), str.size());
}

}

PageBuilder& PageBuilder::Add(const std::string& key, const std::string& value) {
  keys_.push_back(key);
  values_.push_back(value);
  return *this;
}

void PageBuilder::Serialize(std::ostream& os) {
  int64_t size = keys_.size();
  WriteInt(os, keys_.size());

  for (const auto& key : keys_) {
    WriteString(os, key);
  }

  for (const auto& value : values_) {
    WriteString(os, value);
  }
}

void PageBuilder::Clear() {
  keys_.clear();
  values_.clear();
}

}
