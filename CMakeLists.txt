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

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

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

add_library (cachecache STATIC ${SRC})
add_executable (kv_store src/main.cc)
add_executable (supervisor ${SRC_SUPER} src/supervisor_main.cc)
add_executable (cache ${SRC_INST} src/instance_main.cc)

target_link_libraries(supervisor cachecache rd_utils)
target_link_libraries(cache cachecache rd_utils)
target_link_libraries(kv_store cachecache rd_utils)

# UNIT TESTING
enable_testing()

file(
  GLOB_RECURSE
  TESTS
  tests/*.cc
)

add_executable(tests ${TESTS} ${SRC})
target_link_libraries(tests GTest::gtest_main cachecache rd_utils)
target_include_directories(tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(GoogleTest)
gtest_discover_tests(tests)
