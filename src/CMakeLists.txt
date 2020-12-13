file(GLOB_RECURSE files RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.c[p]*)
list(REMOVE_ITEM files main.cpp)

add_library(ini STATIC ${files})
target_include_directories(ini PUBLIC ${dirs})

add_executable(lini main.cpp)
target_link_libraries(lini ini)

install(TARGETS lini
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT runtime)

set(files ${files} PARENT_SCOPE)
