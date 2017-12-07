cmake_minimum_required(VERSION 3.1)
project(TestFindXalanC CXX)
include(CTest)

find_package(XalanC REQUIRED)

add_executable(test_xalanc_tgt main.cxx)
target_link_libraries(test_xalanc_tgt XalanC::XalanC)
add_test(NAME test_xalanc_tgt COMMAND test_xalanc_tgt)

add_executable(test_xalanc_var main.cxx)
target_include_directories(test_xalanc_var PRIVATE ${XalanC_INCLUDE_DIRS})
target_link_libraries(test_xalanc_var PRIVATE ${XalanC_LIBRARIES})
add_test(NAME test_xalanc_var COMMAND test_xalanc_var)
