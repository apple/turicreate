add_test(NAME FindICU.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindICU/Test"
  "${CMake_BINARY_DIR}/Tests/FindICU/Test"
  ${build_generator_args}
  --build-project TestFindICU
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
