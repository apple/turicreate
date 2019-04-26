cmake_minimum_required(VERSION 3.6)
project(TestFindLibUV C)
include(CTest)

# CMake does not actually provide FindLibUV publicly.
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../Source/Modules)

find_package(LibUV REQUIRED)

add_executable(test_libuv_tgt main.c)
target_link_libraries(test_libuv_tgt LibUV::LibUV)
add_test(NAME test_libuv_tgt COMMAND test_libuv_tgt)

add_executable(test_libuv_var main.c)
target_include_directories(test_libuv_var PRIVATE ${LibUV_INCLUDE_DIRS})
target_link_libraries(test_libuv_var PRIVATE ${LibUV_LIBRARIES})
add_test(NAME test_libuv_var COMMAND test_libuv_var)
