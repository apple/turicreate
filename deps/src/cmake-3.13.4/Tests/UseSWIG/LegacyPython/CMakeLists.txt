cmake_minimum_required(VERSION 3.1)

project(TestLegacyPython CXX)

include(CTest)

set(language "python")

include (../LegacyConfiguration.cmake)

add_test (NAME LegacyPython
  COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}${PS}$<TARGET_FILE_DIR:${SWIG_MODULE_example_REAL_NAME}>"
  "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../runme.py")
