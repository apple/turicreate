
cmake_minimum_required (VERSION 2.8.12)
enable_testing()
project(basic_checks)

set(MATLAB_FIND_DEBUG TRUE)

# the success of the following command is dependent on the current configuration:
# - on 32bits builds (cmake is building with 32 bits), it looks for 32 bits Matlab
# - on 64bits builds (cmake is building with 64 bits), it looks for 64 bits Matlab
find_package(Matlab REQUIRED COMPONENTS MX_LIBRARY MAIN_PROGRAM)



matlab_add_mex(
    # target name
    NAME cmake_matlab_test_wrapper1
    # output name
    OUTPUT_NAME cmake_matlab_mex1
    SRC ${CMAKE_CURRENT_SOURCE_DIR}/../matlab_wrapper1.cpp
    DOCUMENTATION ${CMAKE_CURRENT_SOURCE_DIR}/../help_text1.m.txt
  )


matlab_add_unit_test(
    NAME ${PROJECT_NAME}_matlabtest-1
    TIMEOUT 90
    UNITTEST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../cmake_matlab_unit_tests1.m
    ADDITIONAL_PATH $<TARGET_FILE_DIR:cmake_matlab_test_wrapper1>
    )

matlab_add_unit_test(
    NAME ${PROJECT_NAME}_matlabtest-2
    TIMEOUT 15
    UNITTEST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../cmake_matlab_unit_tests_timeout.m
    ADDITIONAL_PATH $<TARGET_FILE_DIR:cmake_matlab_test_wrapper1>
    )
set_tests_properties(${PROJECT_NAME}_matlabtest-2 PROPERTIES WILL_FAIL TRUE)


# testing the test without the unittest framework of Matlab
matlab_add_unit_test(
    NAME ${PROJECT_NAME}_matlabtest-3
    TIMEOUT 30
    NO_UNITTEST_FRAMEWORK
    UNITTEST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../cmake_matlab_unit_tests2.m
    ADDITIONAL_PATH $<TARGET_FILE_DIR:cmake_matlab_test_wrapper1>
    )

matlab_add_unit_test(
    NAME ${PROJECT_NAME}_matlabtest-4
    TIMEOUT 30
    NO_UNITTEST_FRAMEWORK
    UNITTEST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../cmake_matlab_unit_tests3.m
    ADDITIONAL_PATH $<TARGET_FILE_DIR:cmake_matlab_test_wrapper1>
    )
set_tests_properties(${PROJECT_NAME}_matlabtest-4 PROPERTIES WILL_FAIL TRUE)
