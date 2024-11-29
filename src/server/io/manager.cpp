#include "manager.hpp"

#include <filesystem>

#include <spdlog/spdlog.h>

namespace structuredb::server::io {

namespace {

constexpr int kThreadNum = 2;
}

Manager::Manager(boost::asio::io_context& io_context)
  : io_context_{io_context}
  , blocking_executor_{kThreadNum}
{}

Awaitable<FileReader::Ptr> Manager::CreateFileReader(const std::string& path) {
  auto result = std::make_shared<FileReader>(io_context_, blocking_executor_);
  co_await result->Open(path);
  co_return result;
}

Awaitable<FileWriter::Ptr> Manager::CreateFileWriter(const std::string& path, bool append) {
  auto result = std::make_shared<FileWriter>(io_context_, blocking_executor_);
  co_await result->Open(path, append);
  co_return result;
}

Awaitable<void> Manager::CreateDirectory(const std::string& path) {
  SPDLOG_INFO("Going to CreateDirectory: {}", path);
  co_await blocking_executor_.Execute([path]() {
    try {
      const bool created = std::filesystem::create_directory(path);
      SPDLOG_INFO("Directory created: {}, {}", path, created);
    } catch (...) {
      SPDLOG_ERROR("Failed to create directory");
    }
  });
}

Awaitable<std::vector<std::string>> Manager::ListDirectory(const std::string& path) {
  const auto result = co_await blocking_executor_.Execute([&]() {
      std::vector<std::string> result;
      if (!std::filesystem::is_directory(path)) {
        return result;
      }
      for (const auto& item : std::filesystem::directory_iterator(path)) {
        result.push_back(item.path().filename());
      }
      return result;
  });
  co_return result;
}

Awaitable<void> Manager::Remove(const std::string& path) {
  co_await blocking_executor_.Execute([&]() {
      std::filesystem::remove(path);
  });
}

}
