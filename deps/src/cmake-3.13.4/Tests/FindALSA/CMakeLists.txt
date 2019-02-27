add_test(NAME FindALSA.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindALSA/Test"
  "${CMake_BINARY_DIR}/Tests/FindALSA/Test"
  ${build_generator_args}
  --build-project TestFindALSA
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
