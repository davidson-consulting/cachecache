CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
project(socialNetCache)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -Wall")

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

add_compile_definitions(LOG_LEVEL=3) # error = 1, warn = 2, info = 3, debug = 6
add_executable (deployer ${SRC})

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/src/)
target_link_libraries (deployer rd_utils vjoule_cpp ssh)
