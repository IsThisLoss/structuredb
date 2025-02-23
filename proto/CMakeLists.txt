add_library(structuredb-proto table_service.proto transaction_service.proto replication_service.proto)
target_include_directories(structuredb-proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${protobuf_INCLUDE_DIR})
target_link_libraries(structuredb-proto PUBLIC gRPC::grpc++)

protobuf_generate(TARGET structuredb-proto)

protobuf_generate(
  TARGET structuredb-proto
  LANGUAGE grpc
  PLUGIN protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
  PLUGIN_OPTIONS generate_mock_code=true
  GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
)
