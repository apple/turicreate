include(RunCMake)

run_cmake(DoesNotExist)
run_cmake(Missing)
run_cmake(Function)

set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/ExcludeFromAll-build)
set(RunCMake_TEST_NO_CLEAN 1)

file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

run_cmake(ExcludeFromAll)
run_cmake_command(ExcludeFromAll-build ${CMAKE_COMMAND} --build .)

unset(RunCMake_TEST_BINARY_DIR)
unset(RunCMake_TEST_NO_CLEAN)
