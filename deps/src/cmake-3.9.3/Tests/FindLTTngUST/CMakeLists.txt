add_test(NAME FindLTTngUST.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindLTTngUST/Test"
  "${CMake_BINARY_DIR}/Tests/FindLTTngUST/Test"
  ${build_generator_args}
  --build-project TestFindLTTngUST
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
