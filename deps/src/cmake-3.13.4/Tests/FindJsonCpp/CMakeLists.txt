add_test(NAME FindJsonCpp.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindJsonCpp/Test"
  "${CMake_BINARY_DIR}/Tests/FindJsonCpp/Test"
  ${build_generator_args}
  --build-project TestFindJsonCpp
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
