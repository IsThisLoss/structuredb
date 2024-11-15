#include "page.hpp"

#include <iostream>

namespace structuredb::server::lsm::disk {

Awaitable<int64_t> LoadInt(io::FileReader& is) {
  int64_t size{0};
  co_await is.Read(reinterpret_cast<char*>(&size), sizeof(int64_t));
  std::cerr << "Read int: " << size << std::endl;
  co_return size;
}

Awaitable<std::string> LoadString(io::FileReader& is) {
  const int64_t size = co_await LoadInt(is);
  std::string result;
  result.resize(size);
  co_await is.Read(result.data(), size);
  std::cerr << "Loaded string: " << result << std::endl;
  co_return result;
}

Awaitable<Page> Page::Load(io::FileReader& file_reader) {
  Page page{};
  page.size_ = co_await LoadInt(file_reader);
  std::cerr << "Load page with size: " << page.size_ << std::endl;
  page.keys_.reserve(page.size_);
  page.values_.reserve(page.size_);
  for (int i = 0; i < page.size_; i++) {
    page.keys_.push_back(co_await LoadString(file_reader));
    page.values_.push_back(co_await LoadString(file_reader));
  }
  co_return page;
}

std::optional<std::string> Page::Find(const std::string& key) const {
  auto it = std::lower_bound(keys_.begin(), keys_.end(), key);
  if (it == keys_.end() || *it != key) {
    return std::nullopt;
  }
  const auto offset = std::distance(keys_.begin(), it);
  auto result_it = std::next(values_.begin(), offset);
  return std::make_optional(*result_it);
}

const std::string& Page::MinKey() const {
  return keys_.front();
}

const std::string& Page::MaxKey() const {
  return keys_.back();
}

}
