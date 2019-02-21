add_test(NAME FindBoost.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindBoost/Test"
  "${CMake_BINARY_DIR}/Tests/FindBoost/Test"
  ${build_generator_args}
  --build-project TestFindBoost
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )

add_test(NAME FindBoost.TestFail COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindBoost/TestFail"
  "${CMake_BINARY_DIR}/Tests/FindBoost/TestFail"
  ${build_generator_args}
  --build-project TestFailFindBoost
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )

set_tests_properties(FindBoost.TestFail PROPERTIES
  PASS_REGULAR_EXPRESSION "Could not find the following Boost libraries:[ \t\n]+boost_foobar")

add_test(NAME FindBoost.TestHeaders COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindBoost/TestHeaders"
  "${CMake_BINARY_DIR}/Tests/FindBoost/TestHeaders"
  ${build_generator_args}
  --build-project TestFindBoostHeaders
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
