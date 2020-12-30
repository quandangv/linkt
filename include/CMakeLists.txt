list(APPEND dirs ${CMAKE_CURRENT_LIST_DIR})
file(GLOB_RECURSE public_headers ABSOLUTE ${CMAKE_CURRENT_LIST_DIR} *.hpp)

set(dirs ${dirs} PARENT_SCOPE)
set(public_headers ${public_headers} PARENT_SCOPE)
