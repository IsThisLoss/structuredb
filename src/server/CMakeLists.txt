file(
  GLOB_RECURSE
  SERVER_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(structuredb-server ${SERVER_SRCS})

target_link_libraries(
  structuredb-server
  PUBLIC
  structuredb-proto
  Boost::headers
  Boost::boost
  Boost::filesystem
  Boost::coroutine
  Boost::system
  Boost::thread
  Boost::system
  Boost::boost
  grpc++_reflection
  pthread
)
