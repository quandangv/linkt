cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(lini VERSION 1.1.0)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
include(cmake/targets)
include(file_list)

auto_targets(lini "${HEADER_DIRS}" "${INCLUDE_DIR}" "${SOURCES}" "${HEADERS}" "") 
find_library(cspace cspace REQUIRED)
target_link_libraries(lini ${cspace})

if(BUILD_TESTS)
  include(cmake/test)
  add_unit_tests(${TEST_DIR} "${INTERNAL_TESTS}" "${EXTERNAL_TESTS}" "${TEST_FILES}")
endif()

unset(DEBUG_SCOPES CACHE)

