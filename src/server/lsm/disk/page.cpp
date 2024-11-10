#include "page.hpp"

namespace structuredb::server::lsm::disk {

namespace {

int64_t LoadInt(std::istream& is) {
  int64_t size{0};
  is.read(reinterpret_cast<char*>(&size), sizeof(size));
  return size;
}

std::string LoadString(std::istream& is) {
  const int64_t size = LoadInt(is);
  std::string result;
  result.resize(size);
  is.read(result.data(), size);
  return result;
}

}

Page Page::Load(std::istream& is) {
  const int64_t size = LoadInt(is);

  Page result{};
  result.keys_.reserve(size);
  result.values_.reserve(size);

  for (int64_t i = 0; i < size; i++) {
    auto key = LoadString(is);
    result.keys_.push_back(std::move(key));
  }

  for (int64_t i = 0; i < size; i++) {
    auto value = LoadString(is);
    result.values_.push_back(std::move(value));
  }

  return result;
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

}
