cmake_minimum_required(VERSION 3.7)
project(TestFindLibRHash C)
include(CTest)

# CMake does not actually provide FindLibRHash publicly.
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../Source/Modules)

find_package(LibRHash REQUIRED)

add_executable(test_librhash_tgt main.c)
target_link_libraries(test_librhash_tgt LibRHash::LibRHash)
add_test(NAME test_librhash_tgt COMMAND test_librhash_tgt)

add_executable(test_librhash_var main.c)
target_include_directories(test_librhash_var PRIVATE ${LibRHash_INCLUDE_DIRS})
target_link_libraries(test_librhash_var PRIVATE ${LibRHash_LIBRARIES})
add_test(NAME test_librhash_var COMMAND test_librhash_var)
