cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(linked_nodes VERSION 1.1.0)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
include(file_list)
include(cmake/targets)
add_shared_lib("linked_nodes_node" "${NODE_SOURCES}" "${INCLUDE_DIRS};include/node" "${NODE_HEADERS}" "linked_nodes/node")
add_shared_lib("linked_nodes" "${LINI_SOURCES}" "${INCLUDE_DIRS};include" "${LINI_HEADERS}" "linked_nodes")
find_library(cspace cspace REQUIRED)
target_link_libraries(linked_nodes_node ${cspace})
target_link_libraries(linked_nodes linked_nodes_node)

configure_file(${PUBLIC_HEADERS_DIR}/common.hpp.in  ${GENERATED_HEADERS_DIR}/common.hpp)

if(BUILD_TESTS)
  include(cmake/test)
endif()
