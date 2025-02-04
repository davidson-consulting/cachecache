CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
project(socialNetTraceAnalyse)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -Wall")


file(
  GLOB_RECURSE
  SRC
  src/*.cc
  )

add_compile_definitions(LOG_LEVEL=3) # error = 1, warn = 2, info = 3, debug = 6
add_executable (analyser ${SRC})

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries (analyser rd_utils)
