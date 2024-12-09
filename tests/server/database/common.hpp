#include <gtest/gtest.h>

#include <filesystem>
#include <memory>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

#include <database/database.hpp>

namespace structuredb::tests {

class DatabaseTest : public ::testing::Test {
protected:
  void SetUp() override {
    SPDLOG_INFO("DatabaseTest SetUp");
    const auto prefix = ::testing::TempDir();
    const auto dir = server::utils::ToString(server::utils::GenerateUuid());
    tmp_dir_ = "/tmp/structuredb." + dir;
    std::filesystem::create_directory(tmp_dir_);

    io_manager_ = std::make_unique<server::io::Manager>(io_context_);
    db_ = std::make_unique<server::database::Database>(*io_manager_, tmp_dir_);

    asio_thread_ = std::thread([this]() {
        SPDLOG_INFO("Starting asio thread...");
        const auto work_guard = boost::asio::make_work_guard(io_context_);
        try {
          io_context_.run();
        } catch (...) {
          SPDLOG_ERROR("Exception in asio thread");
        }
        SPDLOG_INFO("Exit asio thread");
    });

    Run(db_->Init());
  }

  void TearDown() override {
    SPDLOG_INFO("DatabaseTest TearDown");
    db_.reset();
    io_manager_.reset();
    io_context_.stop();
    asio_thread_.join();
    std::filesystem::remove_all(tmp_dir_);
  }


  template <typename T>
  T Run(boost::asio::awaitable<T>&& coro) {
    auto future = boost::asio::co_spawn(io_context_, std::move(coro), boost::asio::use_future);
    return future.get();
  }

  server::database::Database& GetDatabase() {
    return *db_;
  }

private:
  boost::asio::io_context io_context_{};
  std::string tmp_dir_;

  std::unique_ptr<server::io::Manager> io_manager_;
  std::unique_ptr<server::database::Database> db_;

  std::thread asio_thread_;
};

}