cmake_minimum_required(VERSION 3.8)
project(TestGoogleTest)
include(CTest)

include(GoogleTest)

find_package(GTest REQUIRED)

add_executable(test_gtest1 main1.cxx)
target_link_libraries(test_gtest1 GTest::GTest)

# Simple test of defaults
gtest_add_tests(TARGET test_gtest1
  TEST_LIST testList
)
set(expectedTests
  GoogleTest.LinksAndRuns
  GoogleTest.ConditionalFail
)
if(NOT testList STREQUAL "${expectedTests}")
  message(FATAL_ERROR "Expected test list: ${expectedTests}
Actual test list: ${testList}")
endif()


# Same target, different arguments, so use test prefix and suffix to
# differentiate from the above test cases
gtest_add_tests(TARGET test_gtest1
  TEST_LIST testList
  TEST_PREFIX "set2."
  TEST_SUFFIX ".foo"
  EXTRA_ARGS  --forceFail
)

set(expectedTests
  set2.GoogleTest.LinksAndRuns.foo
  set2.GoogleTest.ConditionalFail.foo
)
if(NOT testList STREQUAL "${expectedTests}")
  message(FATAL_ERROR "Expected test list: ${expectedTests}
Actual test list: ${testList}")
endif()

set_tests_properties(set2.GoogleTest.ConditionalFail.foo PROPERTIES WILL_FAIL YES)


# Search specific sources to get the test list
add_executable(test_gtest2 main2.cxx)
target_link_libraries(test_gtest2 GTest::Main)
gtest_add_tests(TARGET test_gtest2
  TEST_LIST testList
  SOURCES main2.h
)
set(expectedTests
  GoogleTest.SomethingElse
  GoogleTest.OffTest1
  GoogleTest.OffTest2
  GoogleTest.OffTest3
)
if(NOT testList STREQUAL "${expectedTests}")
  message(FATAL_ERROR "Expected test list: ${expectedTests}
Actual test list: ${testList}")
endif()
set(disabledTests
  GoogleTest.OffTest1
  GoogleTest.OffTest2
  GoogleTest.OffTest3
)
foreach(T ${disabledTests})
  get_test_property(${T} DISABLED testDisabled)
  if(NOT testDisabled)
    message(FATAL_ERROR "Test ${T} should be disabled but is not")
  endif()
endforeach()

# Non-keyword form, auto-find sources
add_executable(test_gtest3 main3.cxx)
target_link_libraries(test_gtest3 GTest::Main)
gtest_add_tests(test_gtest3 "" AUTO)
if(NOT TEST GoogleTest.Foo)
  message(FATAL_ERROR "Test case GoogleTest.Foo not defined")
endif()
if(NOT TEST GoogleTest.Bar)
  message(FATAL_ERROR "Test case GoogleTest.Bar not defined")
endif()


# Non-keyword form, explicitly specified sources. Allows a non-target to be
# given for the executable.
add_executable(test_gtest4 main4.cxx)
target_link_libraries(test_gtest4 GTest::Main)
gtest_add_tests($<TARGET_FILE:test_gtest4> "" main4.h)
if(NOT TEST GoogleTest.NoKeywords)
  message(FATAL_ERROR "Test case GoogleTest.NoKeywords not defined")
endif()
