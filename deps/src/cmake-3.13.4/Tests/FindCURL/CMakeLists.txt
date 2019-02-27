add_test(NAME FindCURL.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindCURL/Test"
  "${CMake_BINARY_DIR}/Tests/FindCURL/Test"
  ${build_generator_args}
  --build-project TestFindCURL
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
