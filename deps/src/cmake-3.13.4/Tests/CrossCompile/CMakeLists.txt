cmake_minimum_required (VERSION 2.6)
project(CrossCompile)

unset(run_result CACHE)

#Simulate the cross compile condition
set(CMAKE_CROSSCOMPILING ON)

add_executable(CrossCompile main.c)

try_run(run_result compile_result
    ${CrossCompile_BINARY_DIR}
    ${CrossCompile_SOURCE_DIR}/main.c)
