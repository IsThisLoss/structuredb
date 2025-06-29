cmake_minimum_required(VERSION 3.30)

project(structuredb CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

find_package(Protobuf)
find_package(gRPC)

add_subdirectory(proto)
add_subdirectory(src/server)
add_subdirectory(src/client)
add_subdirectory(src/cli)
add_subdirectory(tests/server)
