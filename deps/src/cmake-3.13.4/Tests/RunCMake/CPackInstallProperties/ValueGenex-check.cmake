include(${RunCMake_SOURCE_DIR}/check.cmake)

test_property("bar/test.cpp" CPACK_TEST_PROP ${EXPECTED_MYTEST_NAME})
