project(test_include_dirs)
include(CTest)
include(GoogleTest)

enable_testing()

add_executable(fake_gtest fake_gtest.cpp)

gtest_discover_tests(
  fake_gtest
  TEST_PREFIX TEST:
  TEST_SUFFIX !1
  EXTRA_ARGS how now "\"brown\" cow"
  PROPERTIES LABELS TEST1
)

gtest_discover_tests(
  fake_gtest
  TEST_PREFIX TEST:
  TEST_SUFFIX !2
  EXTRA_ARGS how now "\"brown\" cow"
  PROPERTIES LABELS TEST2
)

add_executable(no_tests_defined no_tests_defined.cpp)

gtest_discover_tests(
  no_tests_defined
)

# Note change in behavior of TIMEOUT keyword in 3.10.3
# where it was renamed to DISCOVERY_TIMEOUT to prevent it
# from shadowing the TIMEOUT test property. Verify the
# 3.10.3 and later behavior, old behavior added in 3.10.1
# is not supported.
add_executable(property_timeout_test timeout_test.cpp)
target_compile_definitions(property_timeout_test PRIVATE sleepSec=10)

gtest_discover_tests(
  property_timeout_test
  TEST_PREFIX property_
  TEST_SUFFIX _no_discovery
  PROPERTIES TIMEOUT 2
)
gtest_discover_tests(
  property_timeout_test
  TEST_PREFIX property_
  TEST_SUFFIX _with_discovery
  DISCOVERY_TIMEOUT 20
  PROPERTIES TIMEOUT 2
)

add_executable(discovery_timeout_test timeout_test.cpp)
target_compile_definitions(discovery_timeout_test PRIVATE discoverySleepSec=10)
gtest_discover_tests(
  discovery_timeout_test
  TEST_PREFIX discovery_
  DISCOVERY_TIMEOUT 2
)
