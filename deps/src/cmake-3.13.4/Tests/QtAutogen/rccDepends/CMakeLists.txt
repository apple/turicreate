cmake_minimum_required(VERSION 3.7)
project(rccDepends)

set(CMAKE_AUTORCC ON)

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

configure_file(res/input1.txt.in res1/input.txt COPYONLY)
configure_file(res/input1.txt.in res1/inputAdded.txt COPYONLY)
configure_file(res/input2.txt.in res2/input.txt COPYONLY)
configure_file(res/input2.txt.in res2/inputAdded.txt COPYONLY)
# Dependency generated qrc file
add_custom_command(OUTPUT res2.qrc
                   DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/res2.qrc.in
                   COMMAND ${CMAKE_COMMAND} -E sleep 2
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/res2.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/res2.qrc
                   )

add_executable(rccDepends
  main.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/res1.qrc
  ${CMAKE_CURRENT_BINARY_DIR}/res2.qrc )
target_link_libraries(rccDepends ${QT_CORE_TARGET})
add_custom_command(TARGET rccDepends POST_BUILD COMMAND
  ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:rccDepends>" > target.txt)
