cmake_minimum_required(VERSION 3.10)
project(MocMacroName)
include("../AutogenTest.cmake")

# Test CMAKE_AUTOMOC_MACRO_NAMES and AUTOMOC_MACRO_NAMES
list(APPEND CMAKE_AUTOMOC_MACRO_NAMES "QO1_ALIAS")

add_executable(mocMacroName
  main.cpp
  Gadget.cpp
  Object.cpp
  Object1Aliased.cpp
  Object2Aliased.cpp
)
set_property(TARGET mocMacroName PROPERTY AUTOMOC ON)
set_property(TARGET mocMacroName APPEND PROPERTY AUTOMOC_MACRO_NAMES "QO2_ALIAS")
target_link_libraries(mocMacroName ${QT_LIBRARIES})
