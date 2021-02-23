cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(lini VERSION 1.1.0)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
include(cmake/variables)
include(file_list)

include(cmake/targets)
add_shared_lib("node" "${NODE_SOURCES}" "${INCLUDE_DIRS};include/node" "${NODE_HEADERS}" "lini/node")
add_shared_lib("lini" "${LINI_SOURCES}" "${INCLUDE_DIRS};include" "${LINI_HEADERS}" "lini")
find_library(cspace cspace REQUIRED)
target_link_libraries(node ${cspace})
target_link_libraries(lini node)

configure_file(${PRIVATE_HEADERS_DIR}/common.hpp.in  ${GENERATED_HEADERS_DIR}/common.hpp)

if(BUILD_TESTS)
  include(cmake/test)
endif()
