#include <thread>

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <services/table_service/table_service.hpp>
#include <services/transaction_service/transaction_service.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

#include <io/manager.hpp>
#include <database/database.hpp>
#include <cfg/config.hpp>

ABSL_FLAG(std::string, config, "./config.yaml", "Path to config");

void InitLogs(const structuredb::server::cfg::Config& config) {
    std::vector<spdlog::sink_ptr> sinks{};
    if (config.logger.console) {
      auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      sinks.push_back(std::move(console_sink));
    }
    if (config.logger.file.has_value()) {
      auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.logger.file.value());
      sinks.push_back(std::move(file_sink));
    }
    if (sinks.empty()) {
      spdlog::set_level(spdlog::level::off);
      return;
    }
    auto logger = std::make_shared<spdlog::logger>("main", sinks.begin(), sinks.end());
    logger->set_pattern("%^[%x %H:%M:%S.%e] %l %v %@%$");
    logger->set_level(config.logger.level);
    logger->flush_on(config.logger.level);
    spdlog::set_default_logger(std::move(logger));
}

structuredb::server::Awaitable<void> Init(structuredb::server::database::Database& database) {
  try {
    co_await database.Init();
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Failed to initialize database: {}", e.what());
    exit(1);
  }
  SPDLOG_INFO("Database is initialized");
}

int main(int argc, char** argv) {

    const auto args = absl::ParseCommandLine(argc, argv);
    const auto config = structuredb::server::cfg::Parse(absl::GetFlag(FLAGS_config));
  
    InitLogs(config);

    SPDLOG_INFO("Starting...");

    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    grpc::ServerBuilder builder;
    const auto host = std::string{"0.0.0.0:"} + std::to_string(config.port);
    builder.AddListeningPort(host, grpc::InsecureServerCredentials());

    boost::asio::io_context io_context{};
    structuredb::server::io::Manager io_manager{io_context};

    structuredb::server::database::Database database{io_manager, config.root};
    auto init_future = boost::asio::co_spawn(io_context, Init(database), boost::asio::use_future);

    const auto table_service = structuredb::server::services::MakeService(io_manager, database);
    builder.RegisterService(table_service.get());

    const auto transaction_service = structuredb::server::services::MakeTransactionService(io_manager, database);
    builder.RegisterService(transaction_service.get());

    std::thread asio_thread([&io_context]() {
        SPDLOG_INFO("Starting asio thread...");
        const auto guard = boost::asio::make_work_guard(io_context);
        try {
          io_context.run();
        } catch (...) {
          SPDLOG_ERROR("Exception in asio thread");
        }
        SPDLOG_INFO("Starting asio thread");
    });

    init_future.get();

    const auto server = builder.BuildAndStart();
    SPDLOG_INFO("Launch grpc server");
    server->Wait();

    asio_thread.join();

    SPDLOG_INFO("Exit");
    spdlog::shutdown();
    return 0;
}
