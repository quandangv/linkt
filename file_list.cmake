# paths to various directories
get_filename_component(GENERATED_HEADERS_DIR ${CMAKE_BINARY_DIR}/generated-headers ABSOLUTE)
get_filename_component(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include ABSOLUTE)
get_filename_component(PRIVATE_HEADERS_DIR ${CMAKE_SOURCE_DIR}/private-headers ABSOLUTE)
get_filename_component(SRC_DIR ${CMAKE_SOURCE_DIR}/src ABSOLUTE)
get_filename_component(TEST_DIR ${CMAKE_SOURCE_DIR}/test ABSOLUTE)
set(HEADER_DIRS ${INCLUDE_DIR} ${PRIVATE_HEADERS_DIR} ${GENERATED_HEADERS_DIR})

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
set(HEADERS
  ${INCLUDE_DIR}/document.hpp
  ${INCLUDE_DIR}/error.hpp
  ${INCLUDE_DIR}/parse_delink.hpp
  ${INCLUDE_DIR}/string_inter.hpp
  ${INCLUDE_DIR}/string_ref.hpp
)

# source files
set(SOURCES
  ${SRC_DIR}/delink.cpp
  ${SRC_DIR}/document.cpp
  ${SRC_DIR}/execstream.cpp
  ${SRC_DIR}/logger.cpp
  ${SRC_DIR}/parse.cpp
  ${SRC_DIR}/string_ref.cpp
  ${SRC_DIR}/tstring.cpp
)

set(INTERNAL_TESTS tstring_test string_inter execstream_test)
set(EXTERNAL_TESTS parse delink)
set(TEST_FILES delink_file.txt)
