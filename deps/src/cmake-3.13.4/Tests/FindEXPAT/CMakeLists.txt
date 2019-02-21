add_test(NAME FindEXPAT.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindEXPAT/Test"
  "${CMake_BINARY_DIR}/Tests/FindEXPAT/Test"
  ${build_generator_args}
  --build-project TestFindEXPAT
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
