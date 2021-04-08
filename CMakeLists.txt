cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project(linked_nodes VERSION 1.1.0)

list(TRANSFORM CMAKE_MODULE_PATH PREPEND ${CMAKE_CURRENT_SOURCE_DIR})
include(file_list.cmake)
include(cmake/targets.cmake)
add_shared_lib("linked_nodes_node" "${NODE_SOURCES}" "${INCLUDE_DIRS};include/node" "${NODE_HEADERS}" "linked_nodes/node")
add_shared_lib("linked_nodes" "${LINI_SOURCES}" "${INCLUDE_DIRS};include" "${LINI_HEADERS}" "linked_nodes")

if(BUILD_TESTS)
  include(cmake/test.cmake)
endif()
set(BUILD_TESTS OFF)

find_library(CSPACE cspace)
if(CSPACE)
  message("Cspace library found.")
  target_link_libraries(linked_nodes_node ${CSPACE})
else()
  message("Cspace library not found, building it...")
  execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMAND git submodule update --init --rebase lib/cspace)
  add_subdirectory(lib/cspace)
  target_link_libraries(linked_nodes_node cspace)
  target_include_directories(linked_nodes_node PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
  message("Building cspace done!")
endif()

target_link_libraries(linked_nodes linked_nodes_node)

configure_file(${PUBLIC_HEADERS_DIR}/common.hpp.in  ${GENERATED_HEADERS_DIR}/common.hpp)
