include(RunCMake)

set(RunCMake_TEST_OPTIONS "-DCMAKE_INSTALL_PREFIX=${RunCMake_BINARY_DIR}/prefix")

file(REMOVE_RECURSE "${RunCMake_BINARY_DIR}/prefix")
file(MAKE_DIRECTORY "${RunCMake_BINARY_DIR}/prefix/NoPrefix")
file(WRITE "${RunCMake_BINARY_DIR}/prefix/NoPrefix/NoPrefixConfig.cmake" "")
set(RunCMake_TEST_OPTIONS "-DCMAKE_INSTALL_PREFIX:PATH=${RunCMake_BINARY_DIR}/prefix")
run_cmake(with_install_prefix)

file(REMOVE_RECURSE "${RunCMake_BINARY_DIR}/prefix")
file(MAKE_DIRECTORY "${RunCMake_BINARY_DIR}/prefix/NoPrefix")
file(WRITE "${RunCMake_BINARY_DIR}/prefix/NoPrefix/NoPrefixConfig.cmake" "")
list(APPEND RunCMake_TEST_OPTIONS "-DCMAKE_FIND_NO_INSTALL_PREFIX=1")
run_cmake(no_install_prefix)
