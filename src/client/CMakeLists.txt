file(
  GLOB_RECURSE
  CLIENT_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

add_library(structuredb-client ${CLIENT_SRCS})

target_include_directories(structuredb-client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  structuredb-client
  PUBLIC
  structuredb-proto
)
