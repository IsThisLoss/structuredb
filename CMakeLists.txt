cmake_minimum_required(VERSION 3.30)

project(structuredb CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED COMPONENTS asio system thread)
include_directories(${Boost_INCLUDE_DIRS})

find_package(Protobuf)
find_package(gRPC)

add_subdirectory(proto)
add_subdirectory(src/server)
add_subdirectory(src/cli)
