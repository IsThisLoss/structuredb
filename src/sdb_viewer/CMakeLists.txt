file(
  GLOB_RECURSE
  SDB_VIEWER_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

add_executable(structuredb-sdb-viewer ${SDB_VIEWER_SRCS})

target_include_directories(structuredb-sdb-viewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  structuredb-sdb-viewer
  PUBLIC
  structuredb-server-lib
  absl::flags
  absl::flags_parse
)
