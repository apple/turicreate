add_test(NAME FindFreetype.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindFreetype/Test"
  "${CMake_BINARY_DIR}/Tests/FindFreetype/Test"
  ${build_generator_args}
  --build-project TestFindFreetype
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
