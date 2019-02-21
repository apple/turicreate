cmake_minimum_required(VERSION 3.1)

project(TestPython2 C)

include(CTest)

find_package(Python2 3 QUIET)
if (Python2_FOUND)
  message (FATAL_ERROR "Wrong python version found: ${Python2_VERSION}")
endif()

find_package(Python2 REQUIRED COMPONENTS Interpreter Development)
if (NOT Python2_FOUND)
  message (FATAL_ERROR "Fail to found Python 2")
endif()

Python2_add_library (spam2 MODULE ../spam.c)
target_compile_definitions (spam2 PRIVATE PYTHON2)

add_test (NAME python2_spam2
          COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=$<TARGET_FILE_DIR:spam2>"
          "${Python2_EXECUTABLE}" -c "import spam2; spam2.system(\"cd\")")
