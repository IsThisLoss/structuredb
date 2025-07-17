enable_language(C)

add_library(linenoise STATIC linenoise/linenoise.c linenoise/linenoise.h)

target_include_directories(linenoise PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
