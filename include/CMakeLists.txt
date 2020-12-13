list(APPEND dirs ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_BINARY_DIR}/generated_src)
configure_file(${CMAKE_CURRENT_LIST_DIR}/option.hpp.in 
               ${CMAKE_BINARY_DIR}/generated_src/option.hpp
               ESCAPE_QUOTES)

set(dirs ${dirs} PARENT_SCOPE)
