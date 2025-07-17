file(
  GLOB_RECURSE
  CLI_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

add_executable(structuredb-cli ${CLI_SRCS})

target_include_directories(structuredb-cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  structuredb-cli
  PUBLIC
  structuredb-client
  structuredb-proto
  linenoise
  absl::flags
  absl::flags_parse
)
