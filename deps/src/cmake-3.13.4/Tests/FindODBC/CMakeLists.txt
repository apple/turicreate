add_test(NAME FindODBC.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindODBC/Test"
  "${CMake_BINARY_DIR}/Tests/FindODBC/Test"
  ${build_generator_args}
  --build-project TestFindODBC
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
