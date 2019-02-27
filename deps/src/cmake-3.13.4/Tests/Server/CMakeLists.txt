cmake_minimum_required(VERSION 3.4)
project(Server CXX)

find_package(Python REQUIRED)

macro(do_test bsname file type)
  execute_process(COMMAND ${Python_EXECUTABLE}
    -B # no .pyc files
    "${CMAKE_SOURCE_DIR}/${type}-test.py"
    "${CMAKE_COMMAND}"
    "${CMAKE_SOURCE_DIR}/${file}"
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}"
    "${CMAKE_GENERATOR}"
    RESULT_VARIABLE test_result
    )

  if (NOT test_result EQUAL 0)
    message(SEND_ERROR "TEST FAILED: ${test_result}")
  endif()
endmacro()

do_test("test_cache" "tc_cache.json" "server")
do_test("test_handshake" "tc_handshake.json" "server")
do_test("test_globalSettings" "tc_globalSettings.json" "server")
do_test("test_buildsystem1" "tc_buildsystem1.json" "server")

add_executable(Server empty.cpp)
