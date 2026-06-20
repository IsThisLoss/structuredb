#include <memory>
#include <string>
#include <vector>
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
#include <services/replication_service/replication_service.hpp>
#include <replication/follower.hpp>
#include <replication/follower_registry.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

#include <cfg/config.hpp>
#include <database/database.hpp>
#include <database/jobs/compaction.hpp>
#include <database/jobs/flush.hpp>
#include <database/jobs/job_launcher.hpp>
#include <database/jobs/wal_cleaner.hpp>
#include <io/manager.hpp>
#include <lsm/options.hpp>

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

    const structuredb::server::lsm::Options lsm_options{
      .max_records_in_mem_table = config.lsm.max_records_in_mem_table,
      .max_ro_mem_tables = config.lsm.max_ro_mem_tables,
      .page_size = config.lsm.page_size,
      .page_cache_capacity = config.lsm.page_cache_capacity,
    };
    structuredb::server::database::Database database{io_manager, config.root, lsm_options};
    auto init_future = boost::asio::co_spawn(io_context, Init(database), boost::asio::use_future);

    const bool read_only =
        config.replication.role == structuredb::server::cfg::ReplicationRole::kFollower;
    const auto table_service = structuredb::server::services::MakeService(io_manager, database, read_only);
    builder.RegisterService(table_service.get());

    const auto transaction_service = structuredb::server::services::MakeTransactionService(io_manager, database);
    builder.RegisterService(transaction_service.get());

    const auto followers = std::make_shared<structuredb::server::replication::FollowerRegistry>();
    std::unique_ptr<grpc::Service> replication_service;
    if (config.replication.role == structuredb::server::cfg::ReplicationRole::kLeader) {
      replication_service = structuredb::server::services::MakeReplicationService(
          io_manager, config.root + "/wal", config.replication.poll_interval, followers);
      builder.RegisterService(replication_service.get());
      SPDLOG_INFO("Replication: serving as leader");
    }

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

    // background jobs
    structuredb::server::database::JobLauncher job_launcher{io_manager};
    job_launcher.Launch(std::make_shared<structuredb::server::database::Compaction>(database, config.compaction.interval));
    job_launcher.Launch(std::make_shared<structuredb::server::database::Flush>(database, config.flush.interval));
    job_launcher.Launch(std::make_shared<structuredb::server::database::WalCleaner>(io_manager, database, config.root + "/wal", config.wal.clean.interval, followers));

    // follower: stream and apply the leader's WAL on a dedicated thread
    std::unique_ptr<std::thread> follower_thread;
    if (config.replication.role == structuredb::server::cfg::ReplicationRole::kFollower) {
      auto follower = std::make_shared<structuredb::server::replication::Follower>(
          io_manager, database, config.replication.leader_address, config.root + "/wal");
      follower_thread = std::make_unique<std::thread>([follower]() { follower->Run(); });
      follower_thread->detach();
      SPDLOG_INFO("Replication: serving as follower of {}", config.replication.leader_address);
    }

    // server
    const auto server = builder.BuildAndStart();
    SPDLOG_INFO("Launch grpc server");
    server->Wait();

    asio_thread.join();

    SPDLOG_INFO("Exit");
    spdlog::shutdown();
    return 0;
}
