add_test(NAME FindLibUV.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindLibUV/Test"
  "${CMake_BINARY_DIR}/Tests/FindLibUV/Test"
  ${build_generator_args}
  --build-project TestFindLibUV
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
