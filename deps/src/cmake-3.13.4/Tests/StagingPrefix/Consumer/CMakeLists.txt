
cmake_minimum_required (VERSION 2.8.12)
project(Consumer)


add_executable(executable main.cpp)
find_package(Foo CONFIG REQUIRED)
target_link_libraries(executable Foo::foo)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(Bar MODULE REQUIRED)
include_directories(${Bar_INCLUDE_DIRS})
target_link_libraries(executable ${Bar_LIBRARIES})

install(TARGETS executable DESTINATION bin)

if(NOT WIN32)
  find_path(IGNORED_INCLUDE_DIR ignored.h)
  if (IGNORED_INCLUDE_DIR)
    message(SEND_ERROR "Should not find this file. The search path should be excluded.")
  endif()
endif()
