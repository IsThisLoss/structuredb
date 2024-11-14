#include <thread>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <services/table_service/table_service.hpp>

#include <boost/asio/io_context.hpp>

#include <io/manager.hpp>

int main(int argc, const char** argv)
{
    std::cerr << "Starting...\n";
    const auto port = argc >= 2 ? argv[1] : "50051";
    const auto host = std::format("0.0.0.0:{}", port);

    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    grpc::ServerBuilder builder;
    builder.AddListeningPort(host, grpc::InsecureServerCredentials());

    boost::asio::io_context io_context{};
    structuredb::server::io::Manager io_manager{io_context};

    const auto table_service = structuredb::server::services::MakeService(io_manager);
    builder.RegisterService(table_service.get());

    std::thread asio_thread([&io_context]() {
        std::cerr << "Starting asio thread...\n";
        const auto guard = boost::asio::make_work_guard(io_context);
        try {
          io_context.run();
        } catch (...) {
          std::cerr << "Exception in asio thread: "<< std::endl;
        }
        std::cerr << "Exit asio thread...\n";
    });

    const auto server = builder.BuildAndStart();

    std::cerr << "Ready!\n";
    server->Wait();

    asio_thread.join();

    std::cerr << "Exit...\n";
    return 0;
}
