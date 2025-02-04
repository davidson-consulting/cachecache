CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(key_value_store)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g --std=c++17 -O3")

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
  src/cache/*.cc
  src/client/*.cc
  src/instance/*.cc
  src/supervisor/*.cc
  src/utils/*.cc
)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)

add_library (cachecache ${SRC})
add_executable (kv_store src/main.cc)
add_executable (supervisor ${SRC_SUPER} src/supervisor_main.cc)
add_executable (cache ${SRC_INST} src/instance_main.cc)

target_link_libraries(supervisor cachecache rd_utils)
target_link_libraries(cache cachecache rd_utils)
target_link_libraries(kv_store cachecache rd_utils)
