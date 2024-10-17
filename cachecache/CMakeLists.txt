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
  SRC_NO_MAIN
  src/service/*.cc
)

file(  
  GLOB_RECURSE
  SRC
  src/service/*.cc
  )
list(APPEND SRC src/main.cc)

# CACHELIB
find_package(cachelib CONFIG REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)

# Cachecache
add_executable (cachecache ${SRC})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_include_directories(cachecache PUBLIC ${CMAKE_BINARY_DIR}/_deps/rd_utils-src/src)
#target_link_libraries(cachecache cachelib ${CMAKE_BINARY_DIR}/_deps/rd_utils-build/librd_utils.a)
target_link_libraries(cachecache cachelib rd_utils)
