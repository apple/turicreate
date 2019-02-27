enable_language(CXX)

add_library(testLib empty_vs6_1.cpp)
add_executable(testExe empty_vs6_2.cpp)
target_link_libraries(testExe testLib)

export(TARGETS testExe FILE "${CMAKE_CURRENT_BINARY_DIR}/cmp0022NOWARN-exe.cmake")
