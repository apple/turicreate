cmake_minimum_required(VERSION 3.1...3.13)

project(TestBasicPerl CXX)

include(CTest)

set(language "perl")

include (../BasicConfiguration.cmake)

add_test (NAME BasicPerl
  COMMAND "${PERL_EXECUTABLE}" "-I${CMAKE_CURRENT_BINARY_DIR}"
                               "-I$<TARGET_FILE_DIR:example>"
  "${CMAKE_CURRENT_SOURCE_DIR}/../runme.pl")
