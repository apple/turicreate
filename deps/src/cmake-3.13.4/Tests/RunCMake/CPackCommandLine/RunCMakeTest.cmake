include(RunCMake)
set(RunCMake_TEST_TIMEOUT 60)

file(WRITE "${RunCMake_BINARY_DIR}/NotAGenerator-build/CPackConfig.cmake" [[
set(CPACK_PACKAGE_NAME "Test")
set(CPACK_PACKAGE_VERSION "1")
]])
set(RunCMake_TEST_NO_CLEAN 1)
run_cmake_command(NotAGenerator ${CMAKE_CPACK_COMMAND} -G NotAGenerator)
unset(RunCMake_TEST_NO_CLEAN)
