add_test(NAME FindPython.Python2 COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindPython/Python2"
  "${CMake_BINARY_DIR}/Tests/FindPython/Python2"
  ${build_generator_args}
  --build-project TestPython2
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )

add_test(NAME FindPython.Python2Fail COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindPython/Python2Fail"
  "${CMake_BINARY_DIR}/Tests/FindPython/Python2Fail"
  ${build_generator_args}
  --build-project TestPython2Fail
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
set_tests_properties(FindPython.Python2Fail PROPERTIES
  PASS_REGULAR_EXPRESSION "Could NOT find Python2 \\(missing: foobar\\)")

add_test(NAME FindPython.Python3 COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindPython/Python3"
  "${CMake_BINARY_DIR}/Tests/FindPython/Python3"
  ${build_generator_args}
  --build-project TestPython3
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )

add_test(NAME FindPython.Python3Fail COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindPython/Python3Fail"
  "${CMake_BINARY_DIR}/Tests/FindPython/Python3Fail"
  ${build_generator_args}
  --build-project TestPython3Fail
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
set_tests_properties(FindPython.Python3Fail PROPERTIES
  PASS_REGULAR_EXPRESSION "Could NOT find Python3 \\(missing: foobar\\)")

add_test(NAME FindPython.Python COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindPython/Python"
  "${CMake_BINARY_DIR}/Tests/FindPython/Python"
  ${build_generator_args}
  --build-project TestPython
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )

add_test(NAME FindPython.MultiplePackages COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindPython/MultiplePackages"
  "${CMake_BINARY_DIR}/Tests/FindPython/MultiplePackages"
  ${build_generator_args}
  --build-project TestMultiplePackages
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
