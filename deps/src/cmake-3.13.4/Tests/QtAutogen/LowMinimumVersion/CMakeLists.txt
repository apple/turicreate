# Use a low minimum version
cmake_minimum_required(VERSION 3.0)
project(LowMinimumVersion)
include("../AutogenTest.cmake")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
add_executable(lowMinimumVersion
  main.cpp
  item.hpp
  item.cpp
  view.ui
  someText.txt
  example.qrc)
target_link_libraries(lowMinimumVersion ${QT_QTCORE_TARGET})
