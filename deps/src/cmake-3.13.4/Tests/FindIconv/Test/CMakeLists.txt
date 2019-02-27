cmake_minimum_required(VERSION 3.10)
project(TestFindIconv CXX)
include(CTest)

find_package(Iconv REQUIRED)

add_executable(test_iconv_tgt main.cxx)
target_link_libraries(test_iconv_tgt Iconv::Iconv)
add_test(NAME test_iconv_tgt COMMAND test_iconv_tgt)

add_executable(test_iconv_var main.cxx)
target_include_directories(test_iconv_var PRIVATE ${Iconv_INCLUDE_DIRS})
target_link_libraries(test_iconv_var PRIVATE ${Iconv_LIBRARIES})
add_test(NAME test_iconv_var COMMAND test_iconv_var)
