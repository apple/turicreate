project(MyExperimentalFeature)
cmake_minimum_required(VERSION 3.8)

include(CTest)

set(CMAKE_DIRECTORY_LABELS "MyExperimentalFeature;NotASubproject")

add_executable(testapp experimental.c)

add_test(experimentalFail1 testapp 5)
set_tests_properties (experimentalFail1
  PROPERTIES PASS_REGULAR_EXPRESSION "Test!"
  )

add_test(experimentalFail2 testapp -5)
set_tests_properties (experimentalFail2
  PROPERTIES PASS_REGULAR_EXPRESSION "Test!"
  )

add_test(experimentalFail3 testapp -5)
set_tests_properties (experimentalFail3
  PROPERTIES PASS_REGULAR_EXPRESSION "Test!"
  )

add_test(experimentalFail4 testapp -5)
set_tests_properties (experimentalFail4
  PROPERTIES PASS_REGULAR_EXPRESSION "Test!"
  )

add_test(experimentalFail5 testapp -5)
set_tests_properties (experimentalFail5
  PROPERTIES PASS_REGULAR_EXPRESSION "Test!"
  )
