# Load strings submodule
get_filename_component(LIBRARY_DIR ${CMAKE_CURRENT_LIST_DIR}/lib ABSOLUTE)
execute_process(COMMAND git submodule update --init --rebase -- ${LIBRARY_DIR}/strings)

include(lib/strings/file_list)
set(STRINGS_PUBLIC_HEADERS_DIR ${PUBLIC_HEADERS_DIR})
list(APPEND INCLUDE_DIRS ${STRINGS_PUBLIC_HEADERS_DIR})

# paths to various directories
get_filename_component(GENERATED_HEADERS_DIR ${CMAKE_BINARY_DIR}/generated-headers ABSOLUTE)
get_filename_component(PUBLIC_HEADERS_DIR    ${CMAKE_CURRENT_LIST_DIR}/include ABSOLUTE)
get_filename_component(PRIVATE_HEADERS_DIR   ${CMAKE_CURRENT_LIST_DIR}/private-headers ABSOLUTE)
get_filename_component(SRC_DIR               ${CMAKE_CURRENT_LIST_DIR}/src ABSOLUTE)
get_filename_component(TEST_DIR              ${CMAKE_CURRENT_LIST_DIR}/test ABSOLUTE)
get_filename_component(LIBRARY_DIR           ${CMAKE_CURRENT_LIST_DIR}/lib ABSOLUTE)
list(APPEND INCLUDE_DIRS ${PUBLIC_HEADERS_DIR} ${PRIVATE_HEADERS_DIR} ${GENERATED_HEADERS_DIR})

unset(DEBUG_SCOPES CACHE)

# public headers
set(PUBLIC_HEADERS
  ${PUBLIC_HEADERS_DIR}/document.hpp
  ${PUBLIC_HEADERS_DIR}/error.hpp
  ${PUBLIC_HEADERS_DIR}/languages.hpp
  ${PUBLIC_HEADERS_DIR}/string_ref.hpp
  ${PUBLIC_HEADERS_DIR}/container.hpp
  ${STRINGS_PUBLIC_HEADERS_DIR}/tstring.hpp
)

# source files
set(SOURCES
  ${SRC_DIR}/document.cpp
  ${SRC_DIR}/lang_ini.cpp
  ${SRC_DIR}/lang_yml.cpp
  ${SRC_DIR}/container.cpp
  ${SRC_DIR}/string_ref.cpp
  ${SRC_DIR}/string_interpolate_ref.cpp
  ${TSTRING_SOURCES}
)

set(INTERNAL_TESTS)
set(EXTERNAL_TESTS container languages)
set(COPIED_FILES
  key_file.txt
  assign_test.txt
  ini_test.txt
  ini_test_output.txt
  yml_test.txt
  lemonbar_test.txt
  lemonbar_test_output.txt
  lemonbar_untangled_test.txt
)
