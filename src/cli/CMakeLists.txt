file(
  GLOB_RECURSE
  CLI_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

add_executable(structuredb-cli ${CLI_SRCS})

target_link_libraries(
  structuredb-cli
  PUBLIC
  structuredb-client
  absl::flags
  absl::flags_parse
)
