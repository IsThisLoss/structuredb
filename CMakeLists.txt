cmake_minimum_required(VERSION 3.30)

project(structuredb CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(asio-grpc REQUIRED)

add_subdirectory(proto)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC structuredb-proto asio-grpc::asio-grpc)
