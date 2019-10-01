add_test(NAME FindOpenCL.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindOpenCL/Test"
  "${CMake_BINARY_DIR}/Tests/FindOpenCL/Test"
  ${build_generator_args}
  --build-project TestFindOpenCL
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
