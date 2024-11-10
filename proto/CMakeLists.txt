asio_grpc_protobuf_generate(
    GENERATE_GRPC GENERATE_MOCK_CODE
    OUT_VAR "STRUCTUREDB_GRPC_PROTO_SOURCES"
    OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
    IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}"
    PROTOS "${CMAKE_CURRENT_LIST_DIR}/table_service.proto"
)

add_library(structuredb-proto OBJECT)

target_include_directories(structuredb-proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${protobuf_INCLUDE_DIR})

target_sources(structuredb-proto PRIVATE ${STRUCTUREDB_GRPC_PROTO_SOURCES})

target_link_libraries(structuredb-proto PUBLIC gRPC::grpc++ protobuf::libprotobuf)

target_include_directories(
  structuredb-proto
  SYSTEM
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
)
