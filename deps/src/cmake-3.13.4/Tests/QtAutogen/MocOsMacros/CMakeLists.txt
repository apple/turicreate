cmake_minimum_required(VERSION 3.11)
project(MocOsMacros)
include("../AutogenTest.cmake")

# Tests if moc processes Q_OS_XXX macros

message( "Qt5Core_VERSION: ${Qt5Core_VERSION}" )
message(
  "CMAKE_CXX_COMPILER_PREDEFINES_COMMAND: "
  ${CMAKE_CXX_COMPILER_PREDEFINES_COMMAND} )

# On some platforms (e.g. MAC) Q_OS_XXX requires moc to include moc_predefs.h
# which is supported since Qt 5.8 and requires
# CMAKE_CXX_COMPILER_PREDEFINES_COMMAND to be defined.
if( ( ${Qt5Core_VERSION} VERSION_GREATER_EQUAL "5.8" ) AND
  DEFINED CMAKE_CXX_COMPILER_PREDEFINES_COMMAND
)
  message( "Test enabled!" )
  message(
    "CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES: "
    ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES} )

  set(CMAKE_AUTOMOC True)
  add_executable(mocOsMacros
    main.cpp
    TestClass.cpp
    TestClass.hpp
  )
  target_link_libraries(mocOsMacros PRIVATE ${QT_QTCORE_TARGET})
else()
  message( "Test disabled!" )
endif()
