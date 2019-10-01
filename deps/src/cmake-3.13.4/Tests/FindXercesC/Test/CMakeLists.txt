cmake_minimum_required(VERSION 3.1)
project(TestFindXercesC CXX)
include(CTest)

find_package(XercesC REQUIRED)

add_executable(test_xercesc_tgt main.cxx)
target_link_libraries(test_xercesc_tgt XercesC::XercesC)
add_test(NAME test_xercesc_tgt COMMAND test_xercesc_tgt)

add_executable(test_xercesc_var main.cxx)
target_include_directories(test_xercesc_var PRIVATE ${XercesC_INCLUDE_DIRS})
target_link_libraries(test_xercesc_var PRIVATE ${XercesC_LIBRARIES})
add_test(NAME test_xercesc_var COMMAND test_xercesc_var)
