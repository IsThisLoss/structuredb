file(
  GLOB_RECURSE
  SERVER_TESTS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

find_package(Boost REQUIRED)
find_package(GTest REQUIRED)
find_package(spdlog REQUIRED)

add_executable(structuredb-server-tests ${SERVER_TESTS_SRCS})

include_directories(${Boost_INCLUDE_DIRS})
target_include_directories(structuredb-server-tests PRIVATE ${PROJECT_SOURCE_DIR}/src/server)

target_link_libraries(
  structuredb-server-tests
  PUBLIC
  gtest::gtest
  structuredb-server-lib
)

add_test(structuredb-server-tests structuredb-server-tests)
