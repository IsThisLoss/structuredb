#include <thread>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <spdlog/spdlog.h>

#include <services/table_service/table_service.hpp>
#include <services/transaction_service/transaction_service.hpp>

#include <boost/asio/io_context.hpp>

#include <io/manager.hpp>
#include <database/database.hpp>

int main(int argc, const char** argv) {
    spdlog::set_pattern("[%c] %l %v");

    spdlog::info("Starting..");
    const auto port = argc >= 2 ? argv[1] : "50051";
    const auto host = std::string{"0.0.0.0:"} + port;

    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    grpc::ServerBuilder builder;
    builder.AddListeningPort(host, grpc::InsecureServerCredentials());

    boost::asio::io_context io_context{};
    structuredb::server::io::Manager io_manager{io_context};

    structuredb::server::database::Database database{io_manager, "/tmp/db"};

    const auto table_service = structuredb::server::services::MakeService(io_manager, database);
    builder.RegisterService(table_service.get());

    const auto transaction_service = structuredb::server::services::MakeTransactionService(io_manager, database);
    builder.RegisterService(transaction_service.get());

    std::thread asio_thread([&io_context]() {
        spdlog::info("Starting asio thread...");
        const auto guard = boost::asio::make_work_guard(io_context);
        try {
          io_context.run();
        } catch (...) {
          spdlog::error("Exception in asio thread");
        }
        spdlog::info("Starting asio thread");
    });

    const auto server = builder.BuildAndStart();

    spdlog::info("Launch grpc server");
    server->Wait();

    asio_thread.join();

    spdlog::info("Exit");
    return 0;
}
