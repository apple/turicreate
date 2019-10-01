cmake_minimum_required(VERSION 3.1)
project(TestFindODBC C)
include(CTest)

find_package(ODBC)

add_executable(test_odbc_tgt main.c)
target_link_libraries(test_odbc_tgt ODBC::ODBC)
add_test(NAME test_odbc_tgt COMMAND test_odbc_tgt)

add_executable(test_odbc_var main.c)
target_include_directories(test_odbc_var PRIVATE ${ODBC_INCLUDE_DIRS})
target_link_libraries(test_odbc_var PRIVATE ${ODBC_LIBRARIES})
add_test(NAME test_odbc_var COMMAND test_odbc_var)
