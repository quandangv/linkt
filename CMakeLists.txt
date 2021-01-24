cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(lini VERSION 1.1.0)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
include(file_list)

include(cmake/targets)
find_library(cspace cspace REQUIRED)
target_link_libraries(lini ${cspace})

configure_file(${PRIVATE_HEADERS_DIR}/common.hpp.in  ${GENERATED_HEADERS_DIR}/common.hpp)

if(BUILD_TESTS)
  include(cmake/test)
endif()

unset(DEBUG_SCOPES CACHE)

