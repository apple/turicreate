include(${RunCMake_SOURCE_DIR}/check.cmake)

test_property("foo/test.cpp" CPACK_TEST_PROP PROP_VALUE)
