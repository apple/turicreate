add_test(NAME FindIconv.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindIconv/Test"
  "${CMake_BINARY_DIR}/Tests/FindIconv/Test"
  ${build_generator_args}
  --build-project TestFindIconv
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
