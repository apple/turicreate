include(${RunCMake_SOURCE_DIR}/check.cmake)

test_property("multiple_values.txt" CPACK_TEST_PROP "value1;value2;value3")
