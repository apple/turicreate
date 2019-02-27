cmake_minimum_required(VERSION 3.1)

project(TestMultiplePackages C)

find_package (Python2 REQUIRED COMPONENTS Interpreter Development)
find_package (Python3 REQUIRED COMPONENTS Interpreter Development)

# Must find Python 3
find_package (Python REQUIRED)

if (NOT Python3_EXECUTABLE STREQUAL Python_EXECUTABLE)
  message (FATAL_ERROR
    "Python interpreters do not match:\n"
    "  Python_EXECUTABLE='${Python_EXECUTABLE}'\n"
    "  Python3_EXECUTABLE='${Python3_EXECUTABLE}'\n"
    )
endif()


Python2_add_library (spam2 MODULE ../spam.c)
target_compile_definitions (spam2 PRIVATE PYTHON2)

Python3_add_library (spam3 MODULE ../spam.c)
target_compile_definitions (spam3 PRIVATE PYTHON3)


add_test (NAME python2_spam2
          COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=$<TARGET_FILE_DIR:spam3>"
          "${Python2_EXECUTABLE}" -c "import spam2; spam2.system(\"cd\")")

add_test (NAME python3_spam3
          COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=$<TARGET_FILE_DIR:spam3>"
          "${Python3_EXECUTABLE}" -c "import spam3; spam3.system(\"cd\")")
