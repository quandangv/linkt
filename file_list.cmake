# Load strings submodule
include(lib/strings/file_list)
list(APPEND INCLUDE_DIRS ${PUBLIC_HEADERS_DIR})

# paths to various directories
get_filename_component(GENERATED_HEADERS_DIR ${CMAKE_BINARY_DIR}/generated-headers ABSOLUTE)
get_filename_component(PUBLIC_HEADERS_DIR    ${CMAKE_CURRENT_LIST_DIR}/include ABSOLUTE)
get_filename_component(PRIVATE_HEADERS_DIR   ${CMAKE_CURRENT_LIST_DIR}/private-headers ABSOLUTE)
get_filename_component(SRC_DIR               ${CMAKE_CURRENT_LIST_DIR}/src ABSOLUTE)
get_filename_component(TEST_DIR              ${CMAKE_CURRENT_LIST_DIR}/test ABSOLUTE)
get_filename_component(LIBRARY_DIR           ${CMAKE_CURRENT_LIST_DIR}/lib ABSOLUTE)
list(APPEND INCLUDE_DIRS ${PUBLIC_HEADERS_DIR} ${PRIVATE_HEADERS_DIR} ${GENERATED_HEADERS_DIR})

# configure files {{{
  if(PLATFORM EQUAL "Linux")
    add_compile_definitions(PLATFORM_LINUX)
  endif()

  configure_file(${PRIVATE_HEADERS_DIR}/common.hpp.in 
    ${GENERATED_HEADERS_DIR}/common.hpp
    ESCAPE_QUOTES)

  unset(DEBUG_SCOPES CACHE)
# }}}

# public headers
set(PUBLIC_HEADERS
  ${PUBLIC_HEADER_DIR}/document.hpp
  ${PUBLIC_HEADER_DIR}/error.hpp
  ${PUBLIC_HEADER_DIR}/parse_delink.hpp
  ${PUBLIC_HEADER_DIR}/string_ref.hpp
)

message(${TSTRING_SOURCES})
# source files
set(SOURCES
  ${SRC_DIR}/delink.cpp
  ${SRC_DIR}/document.cpp
  ${SRC_DIR}/execstream.cpp
  ${SRC_DIR}/logger.cpp
  ${SRC_DIR}/parse.cpp
  ${SRC_DIR}/string_ref.cpp
  ${TSTRING_SOURCES}
)

set(INTERNAL_TESTS execstream_test)
set(EXTERNAL_TESTS parse delink)
set(COPIED_FILES delink_file.txt)
