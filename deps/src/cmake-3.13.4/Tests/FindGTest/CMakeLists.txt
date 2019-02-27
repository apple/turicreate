add_test(NAME FindGTest.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindGTest/Test"
  "${CMake_BINARY_DIR}/Tests/FindGTest/Test"
  ${build_generator_args}
  --build-project TestFindGTest
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
