#include "file_builder.hpp"

#include <cassert>

namespace structuredb::server::lsm::disk {

FileBuilder::FileBuilder(const std::string& path, const int64_t page_size)
  : page_size_{page_size}
{
  of_.open(path, std::ios::binary);
  assert(of_.is_open());
}

FileBuilder& FileBuilder::Add(const std::string& key, const std::string& value) {

  current_page_.Add(key, value);
  if (++added_ > page_size_) {
    current_page_.Serialize(of_);
    current_page_.Clear();
    added_ = 0;
  }
  return *this;
}

void FileBuilder::Finish() && {
  current_page_.Serialize(of_);
  of_.flush();
  of_.close();
}

}
