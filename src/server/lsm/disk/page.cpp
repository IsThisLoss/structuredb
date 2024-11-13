#include "page.hpp"

namespace structuredb::server::lsm::disk {

namespace {

boost::asio::awaitable<int64_t> LoadInt(io::FileReader& is) {
  int64_t size{0};
  co_await is.Read(reinterpret_cast<char*>(&size), sizeof(size));
  co_return size;
}

boost::asio::awaitable<std::string> LoadString(io::FileReader& is) {
  const int64_t size = co_await LoadInt(is);
  std::string result;
  result.resize(size);
  co_await is.Read(result.data(), size);
  co_return result;
}

}

boost::asio::awaitable<Page> Page::Load(io::FileReader& file_reader) {
  const int64_t size = co_await LoadInt(file_reader);

  Page result{};
  result.keys_.reserve(size);
  result.values_.reserve(size);

  for (int64_t i = 0; i < size; i++) {
    auto key = co_await LoadString(file_reader);
    result.keys_.push_back(std::move(key));
  }

  for (int64_t i = 0; i < size; i++) {
    auto value = co_await LoadString(file_reader);
    result.values_.push_back(std::move(value));
  }

  co_return result;
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
