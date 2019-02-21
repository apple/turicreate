cmake_minimum_required(VERSION 3.1)

project(TestLegacyPerl CXX)

include(CTest)

set(language "perl")

include (../LegacyConfiguration.cmake)

add_test (NAME LegacyPerl
  COMMAND "${PERL_EXECUTABLE}" "-I${CMAKE_CURRENT_BINARY_DIR}"
                               "-I$<TARGET_FILE_DIR:${SWIG_MODULE_example_REAL_NAME}>"
  "${CMAKE_CURRENT_SOURCE_DIR}/../runme.pl")
