add_test(NAME FindOpenGL.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindOpenGL/Test"
  "${CMake_BINARY_DIR}/Tests/FindOpenGL/Test"
  ${build_generator_args}
  --build-project TestFindOpenGL
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
