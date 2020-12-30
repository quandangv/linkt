file(GLOB_RECURSE files RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.c[p]*)

find_library(cspace cspace REQUIRED)

add_library(lini SHARED ${files})
target_link_libraries(lini ${cspace})
target_include_directories(lini PUBLIC ${dirs})

set_target_properties(lini PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(lini PROPERTIES PUBLIC_HEADER "${public_headers}")

message(${CMAKE_INSTALL_INCLUDEDIR}/lini)
install(TARGETS lini
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} 
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lini) 

set(files ${files} PARENT_SCOPE)
