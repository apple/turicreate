enable_language(CXX)

cmake_policy(SET CMP0022 NEW)

add_library(testLib empty_vs6_1.cpp)
add_executable(testExe empty_vs6_2.cpp)
target_link_libraries(testExe testLib)

export(TARGETS testExe FILE "${CMAKE_CURRENT_BINARY_DIR}/cmp0022NEW-exe.cmake")
