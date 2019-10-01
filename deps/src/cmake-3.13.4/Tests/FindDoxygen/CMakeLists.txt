add_test(NAME FindDoxygen.SimpleTest COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindDoxygen/SimpleTest"
  "${CMake_BINARY_DIR}/Tests/FindDoxygen/SimpleTest"
  --build-target allDocTargets
  ${build_generator_args}
  --build-options ${build_options}
)

add_test(NAME FindDoxygen.QuotingTest COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindDoxygen/QuotingTest"
  "${CMake_BINARY_DIR}/Tests/FindDoxygen/QuotingTest"
  --build-target allDocTargets
  ${build_generator_args}
  --build-options ${build_options}
)

add_test(NAME FindDoxygen.AllTarget COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindDoxygen/AllTarget"
  "${CMake_BINARY_DIR}/Tests/FindDoxygen/AllTarget"
  ${build_generator_args}
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
)

if(CMake_TEST_FindDoxygen_Dot)
  add_test(NAME FindDoxygen.DotComponentTest COMMAND
    ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindDoxygen/DotComponentTestTest"
    "${CMake_BINARY_DIR}/Tests/FindDoxygen/DotComponentTestTest"
    ${build_generator_args}
    --build-options ${build_options}
  )
endif()
