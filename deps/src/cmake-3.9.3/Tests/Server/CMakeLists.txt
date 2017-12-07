cmake_minimum_required(VERSION 3.4)
project(Server CXX)

find_package(PythonInterp REQUIRED)

macro(do_test bsname file)
  execute_process(COMMAND ${PYTHON_EXECUTABLE}
    -B # no .pyc files
    "${CMAKE_SOURCE_DIR}/server-test.py"
    "${CMAKE_COMMAND}"
    "${CMAKE_SOURCE_DIR}/${file}"
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}"
    "${CMAKE_GENERATOR}"
    RESULT_VARIABLE test_result
    )

  if (NOT test_result EQUAL 0)
    message(SEND_ERROR "TEST FAILED")
  endif()
endmacro()

do_test("test_handshake" "tc_handshake.json")
do_test("test_globalSettings" "tc_globalSettings.json")
do_test("test_buildsystem1" "tc_buildsystem1.json")

add_executable(Server empty.cpp)
