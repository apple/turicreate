add_test(NAME FindLibRHash.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindLibRHash/Test"
  "${CMake_BINARY_DIR}/Tests/FindLibRHash/Test"
  ${build_generator_args}
  --build-project TestFindLibRHash
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
