cmake_minimum_required(VERSION 3.1...3.13)

project(TestBasicPython CXX)

include(CTest)

set(language "python")

include (../BasicConfiguration.cmake)

add_test (NAME BasicPython
  COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}${PS}$<TARGET_FILE_DIR:example>"
  "${Python_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../runme.py")
