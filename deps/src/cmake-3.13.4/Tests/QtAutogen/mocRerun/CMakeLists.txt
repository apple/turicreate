cmake_minimum_required(VERSION 3.7)
project(mocRerun CXX)

if (QT_TEST_VERSION STREQUAL 4)
  find_package(Qt4 REQUIRED)
  set(QT_CORE_TARGET Qt4::QtCore)
else()
  if (NOT QT_TEST_VERSION STREQUAL 5)
    message(SEND_ERROR "Invalid Qt version specified.")
  endif()

  find_package(Qt5Core REQUIRED)
  set(QT_CORE_TARGET Qt5::Core)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Generated source file
add_custom_command(OUTPUT main.cpp
                   COMMAND ${CMAKE_COMMAND} -E sleep 2
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/main.cpp
                   )

add_executable(mocRerun
  ${CMAKE_CURRENT_BINARY_DIR}/test1.h
  ${CMAKE_CURRENT_BINARY_DIR}/main.cpp
  res1.qrc
  )
set_property(TARGET mocRerun PROPERTY __UNDOCUMENTED_AUTOGEN_GENERATED_FILES 1)
target_include_directories(mocRerun PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mocRerun ${QT_CORE_TARGET})
# Write target name to text file
add_custom_command(TARGET mocRerun POST_BUILD COMMAND
  ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:mocRerun>" > target1.txt)
