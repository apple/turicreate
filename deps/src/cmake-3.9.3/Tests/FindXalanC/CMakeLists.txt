add_test(NAME FindXalanC.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindXalanC/Test"
  "${CMake_BINARY_DIR}/Tests/FindXalanC/Test"
  ${build_generator_args}
  --build-project TestFindXalanC
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
