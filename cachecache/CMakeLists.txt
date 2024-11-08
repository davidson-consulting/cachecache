CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
project(cachecache)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g")
set (CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/../CacheLib/opt/cachelib/lib/cmake\:${CMAKE_PREFIX_PATH\:-}")
set (CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/../CacheLib/")
set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}/../CacheLib/cachelib/cmake")
set (CMAKE_BUILD_TYPE "Debug")

# # RD UTILS
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
  src/cachecache/*.cc
)

# CACHELIB
find_package(cachelib CONFIG REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)

# Cachecache
add_library (cachecache ${SRC})
add_executable (supervisor src/supervisor_main.cc)
# add_dependencies (cachecache rd_utils)

# add_dependencies (supervisor rd_utils)

add_executable (cache src/instance_main.cc)
add_executable (client src/client_main.cc src/cachecache/client/service.cc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)

# add_dependencies (cache rd_utils)
# add_dependencies (client rd_utils)

target_include_directories(cachecache PUBLIC # ${CMAKE_BINARY_DIR}/_deps/rd_utils-src/src
  ${CMAKE_BINARY_DIR}/../CacheLib/opt/cachelib/include)
target_link_libraries(supervisor cachelib cachecache rd_utils)

target_include_directories(cache PUBLIC # ${CMAKE_BINARY_DIR}/_deps/rd_utils-src/src
  ${CMAKE_BINARY_DIR}/../CacheLib/opt/cachelib/include)
target_link_libraries(cache cachelib cachecache rd_utils)

#target_include_directories(client PUBLIC ${CMAKE_BINARY_DIR}/_deps/rd_utils-src/src)
target_link_libraries(client rd_utils)

add_custom_command (TARGET supervisor
                   POST_BUILD
                   COMMAND patchelf --add-needed  libfolly_exception_tracer_base.so.0.58.0-dev supervisor
    )

add_custom_command (TARGET cache
                   POST_BUILD
                   COMMAND patchelf --add-needed  libfolly_exception_tracer_base.so.0.58.0-dev cache
   )

export(TARGETS cachecache NAMESPACE cachecache:: FILE "${CMAKE_CURRENT_BINARY_DIR}/cachecacheConfig.cmake")
