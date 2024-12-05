file(
  GLOB_RECURSE
  SERVER_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(spdlog REQUIRED)
find_package(Boost REQUIRED)
find_package(yaml-cpp REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(structuredb-server ${SERVER_SRCS})

target_link_libraries(
  structuredb-server
  PUBLIC
  Boost::boost
  Boost::coroutine
  Boost::headers
  Boost::system
  absl::flags
  absl::flags_parse
  grpc++_reflection
  spdlog::spdlog
  structuredb-proto
  yaml-cpp
)
