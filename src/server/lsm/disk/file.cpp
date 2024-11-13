#include "file.hpp"

#include <iostream>

namespace structuredb::server::lsm::disk {

boost::asio::awaitable<File> File::Load(boost::asio::io_context& io_context, const std::string& path) {
  File result{};

  io::FileReader file_reader(io_context, path);

  while (true) {
    try {
      auto page = co_await Page::Load(file_reader);
      result.pages_.push_back(std::move(page));
    } catch (const std::exception& e) {
      std::cerr << "Exception: " << e.what();
      break;
    }
  }

  co_return result;
}

std::optional<std::string> File::Find(const std::string& key) const {
  // FIXME use binary search
  for (const auto& page : pages_) {
    const auto value = page.Find(key);
    if (value.has_value()) {
      return value;
    }
  }
  return std::nullopt;
}

}
