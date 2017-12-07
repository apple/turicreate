
cmake_minimum_required (VERSION 2.8.12)
enable_testing()
project(test_should_fail)

find_package(Matlab REQUIRED COMPONENTS MX_LIBRARY)

matlab_add_mex(
    # target name
    NAME cmake_matlab_test_wrapper1
    # output name
    OUTPUT_NAME cmake_matlab_mex1
    SRC ${CMAKE_CURRENT_SOURCE_DIR}/matlab_wrapper1.cpp
  )

# this command should raise a FATAL_ERROR, component MAIN_PROGRAM is missing
matlab_add_unit_test(
    NAME ${PROJECT_NAME}_matlabtest-1
    TIMEOUT 1
    UNITTEST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake_matlab_unit_tests2.m
    ADDITIONAL_PATH $<TARGET_FILE_DIR:cmake_matlab_test_wrapper1>
    )
