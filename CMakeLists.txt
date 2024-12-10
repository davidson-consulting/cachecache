CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(key_value_store)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g --std=c++17")

# include(FetchContent)
# FetchContent_Declare(
#   rd_utils
#   GIT_REPOSITORY git@github.com:davidson-consulting/rd_utils.git
#   GIT_TAG main
# )

# FetchContent_MakeAvailable(rd_utils)

file(
  GLOB_RECURSE
  SRC
  src/*.cc
  )

add_executable (kv_store ${SRC})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_include_directories (kv_store PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../lib/src)

# target_include_directories(kv_store PUBLIC ${CMAKE_BINARY_DIR}/_deps/rd_utils-src/src)
target_link_libraries(kv_store rd_utils)
