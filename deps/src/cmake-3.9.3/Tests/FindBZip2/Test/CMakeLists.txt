cmake_minimum_required(VERSION 3.4)
project(TestFindBZip2 C)
include(CTest)

find_package(BZip2 REQUIRED)

add_definitions(-DCMAKE_EXPECTED_BZip2_VERSION="${BZip2_VERSION_STRING}")

add_executable(test_tgt main.c)
target_link_libraries(test_tgt BZip2::BZip2)
add_test(NAME test_tgt COMMAND test_tgt)

add_executable(test_var main.c)
target_include_directories(test_var PRIVATE ${BZIP2_INCLUDE_DIRS})
target_link_libraries(test_var PRIVATE ${BZIP2_LIBRARIES})
add_test(NAME test_var COMMAND test_var)
