add_test(NAME FindTIFF.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindTIFF/Test"
  "${CMake_BINARY_DIR}/Tests/FindTIFF/Test"
  ${build_generator_args}
  --build-project TestFindTIFF
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
