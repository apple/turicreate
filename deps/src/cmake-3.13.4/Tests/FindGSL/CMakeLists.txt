add_test(NAME FindGSL.rng COMMAND ${CMAKE_CTEST_COMMAND}
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindGSL/rng"
  "${CMake_BINARY_DIR}/Tests/FindGSL/rng"
  ${build_generator_args}
  --build-project FindGSL_rng
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V
  )
