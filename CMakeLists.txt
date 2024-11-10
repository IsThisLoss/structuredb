cmake_minimum_required(VERSION 3.30)

project(structuredb CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost)
find_package(Protobuf)
find_package(gRPC)

add_subdirectory(proto)
add_subdirectory(src/server)
