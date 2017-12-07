add_test(NAME FindDoxygen.SimpleTest COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindDoxygen/SimpleTest"
  "${CMake_BINARY_DIR}/Tests/FindDoxygen/SimpleTest"
  --build-target allDocTargets
  ${build_generator_args}
  --build-options ${build_options}
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
