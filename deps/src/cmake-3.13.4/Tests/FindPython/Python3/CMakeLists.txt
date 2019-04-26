cmake_minimum_required(VERSION 3.1)

project(TestPython3 C)

include(CTest)

find_package(Python3 2 QUIET)
if (Python3_FOUND)
  message (FATAL_ERROR "Wrong python version found: ${Python3_VERSION}")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
if (NOT Python3_FOUND)
  message (FATAL_ERROR "Fail to found Python 3")
endif()

Python3_add_library (spam3 MODULE ../spam.c)
target_compile_definitions (spam3 PRIVATE PYTHON3)

add_test (NAME python3_spam3
          COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=$<TARGET_FILE_DIR:spam3>"
          "${Python3_EXECUTABLE}" -c "import spam3; spam3.system(\"cd\")")
