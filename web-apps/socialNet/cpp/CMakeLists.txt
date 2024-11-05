CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
project(socialNetCache)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall")

include(FetchContent)
fetchcontent_declare(jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG 08bcf77a687fb06e34138e9e9fa12a4ecbe12332 # v0.7.0 release
)
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "disable building examples" FORCE)
fetchcontent_makeavailable(jwt-cpp)


file(
  GLOB_RECURSE
  SRC
  src/*.cc
  )

file(
  GLOB_RECURSE
  SRC_REG
  test/registry.cc
  )

file(
  GLOB_RECURSE
  SRC_FRONT
  test/front.cc
  )

file(
  GLOB_RECURSE
  SRC_REG_LIB
  src/*.cc
  )


list (FILTER SRC_REG_LIB EXCLUDE REGEX "src/main.cc")

add_compile_definitions(LOG_LEVEL=3) # error = 1, warn = 2, info = 3, debug = 6
add_executable (services ${SRC})
add_executable (reg ${SRC_REG} ${SRC_REG_LIB})
add_executable (front ${SRC_FRONT} ${SRC_REG_LIB})

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries (services rd_utils vjoule_cpp mysqlpp mysqlclient jwt-cpp::jwt-cpp httpserver)
target_link_libraries (reg rd_utils vjoule_cpp mysqlpp mysqlclient jwt-cpp::jwt-cpp httpserver)
target_link_libraries (front rd_utils vjoule_cpp mysqlpp mysqlclient jwt-cpp::jwt-cpp httpserver)
