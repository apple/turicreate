add_test(NAME FindOpenSSL.rand COMMAND ${CMAKE_CTEST_COMMAND}
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindOpenSSL/rand"
  "${CMake_BINARY_DIR}/Tests/FindOpenSSL/rand"
  ${build_generator_args}
  --build-project FindOpenSSL_rand
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V
  )
