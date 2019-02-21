cmake_minimum_required(VERSION 3.4)
project(TestFindVulkan C)
include(CTest)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../)
find_package(Vulkan REQUIRED)

add_executable(test_tgt main.c)
target_link_libraries(test_tgt Vulkan::Vulkan)
add_test(NAME test_tgt COMMAND test_tgt)

add_executable(test_var main.c)
target_include_directories(test_var PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(test_var PRIVATE ${Vulkan_LIBRARIES})
add_test(NAME test_var COMMAND test_var)
