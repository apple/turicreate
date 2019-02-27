cmake_minimum_required(VERSION 3.11.2)
project(RccConfigChange)
include("../../AutogenTest.cmake")

# Enable AUTORCC for all targets
set(CMAKE_AUTORCC ON)

# Initial resource files setup
configure_file(resGen/input1.txt.in resGen/input1.txt COPYONLY)
configure_file(resGen/input2.txt.in resGen/input2.txt COPYONLY)
configure_file(resGen.qrc.in resGen.qrc COPYONLY)

# Generated qrc file with dependency
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resGen.qrc
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resGen.qrc.in
  COMMAND ${CMAKE_COMMAND} -E sleep 2
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resGen.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/resGen.qrc
)

# Target that uses a plain .qrc file
add_executable(rccConfigChangePlain main.cpp resPlain.qrc)
target_link_libraries(rccConfigChangePlain ${QT_QTCORE_TARGET})

# Target that uses a GENERATED .qrc file
add_executable(rccConfigChangeGen main.cpp ${CMAKE_CURRENT_BINARY_DIR}/resGen.qrc )
target_link_libraries(rccConfigChangeGen ${QT_QTCORE_TARGET})
