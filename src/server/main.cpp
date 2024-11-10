#include <format>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <services/table_service/table_service.hpp>

int main(int argc, const char** argv)
{
    std::printf("Starting...\n");
    const auto port = argc >= 2 ? argv[1] : "50051";
    const auto host = std::format("0.0.0.0:{}", port);

    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    grpc::ServerBuilder builder;
    builder.AddListeningPort(host, grpc::InsecureServerCredentials());

    const auto table_service = structuredb::server::services::MakeService();
    builder.RegisterService(table_service.get());

    const auto server = builder.BuildAndStart();
    server->Wait();

    std::printf("Exit...\n");
    return 0;
}

