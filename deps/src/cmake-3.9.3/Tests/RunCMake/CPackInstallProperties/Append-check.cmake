include(${RunCMake_SOURCE_DIR}/check.cmake)

test_property("append.txt" CPACK_TEST_PROP "value1;value2;value3")
