cmake_minimum_required(VERSION 3.10)
project(MocBasic)
include("../../AutogenTest.cmake")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Generated source file
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/main.cpp
  COMMAND ${CMAKE_COMMAND} -E sleep 2
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/main.cpp
)

add_executable(mocBasic
  ${CMAKE_CURRENT_BINARY_DIR}/test1.h
  ${CMAKE_CURRENT_BINARY_DIR}/main.cpp
  res1.qrc
)
target_include_directories(mocBasic PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mocBasic ${QT_QTCORE_TARGET})
# Write target name to text file
add_custom_command(TARGET mocBasic POST_BUILD COMMAND
  ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:mocBasic>" > mocBasic.txt
)
