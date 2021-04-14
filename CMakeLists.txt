cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project(linked VERSION 1.1.0)

list(TRANSFORM CMAKE_MODULE_PATH PREPEND ${CMAKE_CURRENT_SOURCE_DIR})
include(file_list.cmake)
include(cmake/targets.cmake)
add_shared_lib("linked_node" "${NODE_SOURCES}" "${INCLUDE_DIRS};include/node" "${NODE_HEADERS}" "linked/node")
add_shared_lib("linked_lang" "${LINI_SOURCES}" "${INCLUDE_DIRS};include" "${LINI_HEADERS}" "linked")

if(BUILD_TESTS)
  include(cmake/test.cmake)
endif()
set(BUILD_TESTS OFF)

find_library(CSPACE cspace)
if(CSPACE)
  message("Cspace library found.")
  target_link_libraries(linked_node ${CSPACE})
else()
  message("Cspace library not found, building it...")
  execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMAND git submodule update --init --rebase lib/cspace)
  execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib/cspace
    COMMAND git submodule update --init --rebase cmake)
  add_subdirectory(lib/cspace)
  target_link_libraries(linked_node cspace)
  target_include_directories(linked_node PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
  message("Building cspace done!")
endif()

target_link_libraries(linked_lang linked_node)
add_library(linked INTERFACE)
target_link_libraries(linked INTERFACE linked_lang)

configure_file(${PUBLIC_HEADERS_DIR}/common.hpp.in  ${GENERATED_HEADERS_DIR}/common.hpp)

add_executable(linked_replace ${SRC_DIR}/linked_replace.cpp)
target_link_libraries(linked_replace linked)
target_include_directories(linked_replace PUBLIC "${PUBLIC_HEADERS_DIR}" "${INCLUDE_DIRS}")
install(TARGETS linked_replace DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT runtime)
set_target_properties(linked_replace PROPERTIES VERSION ${PROJECT_VERSION}
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

