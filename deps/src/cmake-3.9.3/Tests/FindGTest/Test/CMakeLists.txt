cmake_minimum_required(VERSION 3.1)
project(TestFindGTest CXX)
include(CTest)

find_package(GTest REQUIRED)

add_executable(test_gtest_tgt main.cxx)
target_link_libraries(test_gtest_tgt GTest::Main)
add_test(NAME test_gtest_tgt COMMAND test_gtest_tgt)

add_executable(test_gtest_var main.cxx)
target_include_directories(test_gtest_var PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(test_gtest_var PRIVATE ${GTEST_BOTH_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME test_gtest_var COMMAND test_gtest_var)
