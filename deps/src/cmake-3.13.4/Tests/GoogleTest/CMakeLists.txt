add_test(NAME GoogleTest.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/GoogleTest/Test"
  "${CMake_BINARY_DIR}/Tests/GoogleTest/Test"
  ${build_generator_args}
  --build-project TestGoogleTest
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
