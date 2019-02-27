cmake_minimum_required(VERSION 3.1)

project(TestPython3Fail C)

include(CTest)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development foobar)

Python3_add_library (spam3 MODULE ../spam.c)
target_compile_definitions (spam3 PRIVATE PYTHON3)

add_test (NAME python3_spam3
          COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=$<TARGET_FILE_DIR:spam3>"
          "${Python3_EXECUTABLE}" -c "import spam3; spam3.system(\"cd\")")
