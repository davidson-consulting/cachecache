CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(cachecache)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g")

# RD UTILS
include(FetchContent)
FetchContent_Declare(
  rd_utils
  GIT_REPOSITORY git@github.com:davidson-consulting/rd_utils.git
  GIT_TAG main
)

FetchContent_MakeAvailable(rd_utils)

file(
  GLOB_RECURSE
  SRC
  src/cachecache/*.cc
)

# CACHELIB
find_package(cachelib CONFIG REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)

# Cachecache
add_library (cachecache ${SRC})
add_executable (supervisor src/supervisor_main.cc)
add_executable (client src/client_main.cc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)

target_include_directories(cachecache PUBLIC ${CMAKE_BINARY_DIR}/_deps/rd_utils-src/src)
target_include_directories(supervisor PUBLIC ${CMAKE_BINARY_DIR}/_deps/rd_utils-src/src)
target_link_libraries(supervisor cachecache ${CMAKE_BINARY_DIR}/_deps/rd_utils-build/librd_utils.a)

target_include_directories(client PUBLIC ${CMAKE_BINARY_DIR}/_deps/rd_utils-src/src)
target_link_libraries(client cachelib cachecache ${CMAKE_BINARY_DIR}/_deps/rd_utils-build/librd_utils.a)
