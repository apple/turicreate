include(${RunCMake_SOURCE_DIR}/check.cmake)

test_variable(CPACK_BACKSLASH "\\")
test_variable(CPACK_QUOTE "a;b;c")
test_variable(CPACK_DOLLAR "ab")
