add_test(NAME FindPNG.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindPNG/Test"
  "${CMake_BINARY_DIR}/Tests/FindPNG/Test"
  ${build_generator_args}
  --build-project TestFindPNG
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
