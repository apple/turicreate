include(RunCMake)

function(run_GoogleTest)
  # Use a single build tree for a few tests without cleaning.
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/GoogleTest-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  if(NOT RunCMake_GENERATOR_IS_MULTI_CONFIG)
    set(RunCMake_TEST_OPTIONS -DCMAKE_BUILD_TYPE=Debug)
  endif()
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(GoogleTest)

  run_cmake_command(GoogleTest-build
    ${CMAKE_COMMAND}
    --build .
    --config Debug
    --target fake_gtest
  )

  run_cmake_command(GoogleTest-property-timeout-exe
    ${CMAKE_COMMAND}
    --build .
    --config Debug
    --target property_timeout_test
  )

  set(RunCMake_TEST_OUTPUT_MERGE 1)
  run_cmake_command(GoogleTest-discovery-timeout
    ${CMAKE_COMMAND}
    --build .
    --config Debug
    --target discovery_timeout_test
  )
  set(RunCMake_TEST_OUTPUT_MERGE 0)

  run_cmake_command(GoogleTest-test1
    ${CMAKE_CTEST_COMMAND}
    -C Debug
    -L TEST1
    --no-label-summary
  )

  run_cmake_command(GoogleTest-test2
    ${CMAKE_CTEST_COMMAND}
    -C Debug
    -L TEST2
    --no-label-summary
  )

  run_cmake_command(GoogleTest-test-missing
    ${CMAKE_CTEST_COMMAND}
    -C Debug
    -R no_tests_defined
    --no-label-summary
  )

  run_cmake_command(GoogleTest-property-timeout1
    ${CMAKE_CTEST_COMMAND}
    -C Debug
    -R property_timeout\\.case_no_discovery
    --no-label-summary
  )

  run_cmake_command(GoogleTest-property-timeout2
    ${CMAKE_CTEST_COMMAND}
    -C Debug
    -R property_timeout\\.case_with_discovery
    --no-label-summary
  )
endfunction()

run_GoogleTest()
