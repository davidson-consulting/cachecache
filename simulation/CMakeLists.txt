CMAKE_MINIMUM_REQUIRED(VERSION 3.28)
project(cachecache_simulation)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g")

include(FetchContent)

# RD UTILS
FetchContent_Declare(
  rd_utils
  GIT_REPOSITORY git@github.com:davidson-consulting/rd_utils.git
  GIT_TAG gcc13
)

FetchContent_MakeAvailable(rd_utils)

# VCPUSim
FetchContent_Declare(
  vcpusim
  GIT_REPOSITORY git@github.com:davidson-consulting/vCpuSim.git
  GIT_TAG application
  GIT_SHALLOW 1
)
FetchContent_MakeAvailable(vcpusim)

# Cachecache
set(cachecache_DIR ../cachecache/.build)
find_package(cachecache)

file(  
  GLOB_RECURSE
  SRC
  src/*.cc
  )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)

# Cachecache
add_executable (cachecache_simulation ${SRC})
target_include_directories(cachecache_simulation PUBLIC ${CMAKE_BINARY_DIR}/_deps/rd_utils-src/src)
target_include_directories(cachecache_simulation PUBLIC ${CMAKE_BINARY_DIR}/_deps/vcpusim-src/lib/src)
target_link_libraries(cachecache_simulation rd_utils vcpu_sim_lib)
