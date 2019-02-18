add_test(NAME FindPatch.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindPatch/Test"
  "${CMake_BINARY_DIR}/Tests/FindPatch/Test"
  ${build_generator_args}
  --build-options ${build_options}
)
