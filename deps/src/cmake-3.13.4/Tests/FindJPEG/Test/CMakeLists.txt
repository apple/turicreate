cmake_minimum_required(VERSION 3.1)
project(TestFindJPEG C)
include(CTest)

find_package(JPEG)

add_executable(test_jpeg_tgt main.c)
target_link_libraries(test_jpeg_tgt JPEG::JPEG)
add_test(NAME test_jpeg_tgt COMMAND test_jpeg_tgt)

add_executable(test_jpeg_var main.c)
target_include_directories(test_jpeg_var PRIVATE ${JPEG_INCLUDE_DIRS})
target_link_libraries(test_jpeg_var PRIVATE ${JPEG_LIBRARIES})
add_test(NAME test_jpeg_var COMMAND test_jpeg_var)
