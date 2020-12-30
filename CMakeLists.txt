cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(lini VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(PLATFORM EQUAL "Linux")
  add_compile_definitions(PLATFORM_LINUX)
endif()

add_subdirectory(private-headers)
add_subdirectory(include)
add_subdirectory(src)

unset(DEBUG_SCOPES CACHE)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
