add_test(NAME FindXercesC.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindXercesC/Test"
  "${CMake_BINARY_DIR}/Tests/FindXercesC/Test"
  ${build_generator_args}
  --build-project TestFindXercesC
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
