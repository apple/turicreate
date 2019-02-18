cmake_minimum_required(VERSION 3.1)

project(TestPython C)

include(CTest)

find_package(Python 3 REQUIRED COMPONENTS Interpreter Development)
if (NOT Python_FOUND)
  message (FATAL_ERROR "Fail to found Python 3")
endif()

Python_add_library (spam3 MODULE ../spam.c)
target_compile_definitions (spam3 PRIVATE PYTHON3)

add_test (NAME python_spam3
          COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=$<TARGET_FILE_DIR:spam3>"
          "${Python_EXECUTABLE}" -c "import spam3; spam3.system(\"cd\")")
