cmake_minimum_required(VERSION 3.4)
project(TestFindPNG C)
include(CTest)

find_package(PNG REQUIRED)

add_definitions(-DCMAKE_EXPECTED_PNG_VERSION="${PNG_VERSION_STRING}")

add_executable(test_tgt main.c)
target_link_libraries(test_tgt PNG::PNG)
add_test(NAME test_tgt COMMAND test_tgt)

add_executable(test_var main.c)
target_include_directories(test_var PRIVATE ${PNG_INCLUDE_DIRS})
target_link_libraries(test_var PRIVATE ${PNG_LIBRARIES})
add_test(NAME test_var COMMAND test_var)
