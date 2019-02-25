
cmake_minimum_required (VERSION 2.8.12)
enable_testing()
project(failure_reports)

# gather tests about proper failure reports

set(MATLAB_FIND_DEBUG TRUE)

if(IS_MCR)
    set(components MX_LIBRARY)
    set(RUN_UNIT_TESTS FALSE)
else()
    set(RUN_UNIT_TESTS TRUE)
    set(components MX_LIBRARY MAIN_PROGRAM)
endif()

if(NOT "${MCR_ROOT}" STREQUAL "")
    set(Matlab_ROOT_DIR "${MCR_ROOT}")
    if(NOT EXISTS "${MCR_ROOT}")
        message(FATAL_ERROR "MCR does not exist ${MCR_ROOT}")
    endif()
endif()

find_package(Matlab REQUIRED COMPONENTS ${components})

# main extensions for testing, same as other tests
matlab_add_mex(
    # target name
    NAME cmake_matlab_test_wrapper1
    # output name
    OUTPUT_NAME cmake_matlab_mex1
    SRC ${CMAKE_CURRENT_SOURCE_DIR}/../matlab_wrapper1.cpp
    DOCUMENTATION ${CMAKE_CURRENT_SOURCE_DIR}/../help_text1.m.txt
  )

if(RUN_UNIT_TESTS)
    # the unit test file does not exist: the failure should be properly reported
    matlab_add_unit_test(
        NAME ${PROJECT_NAME}_matlabtest-1
        TIMEOUT 300
        UNITTEST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../nonexistantfile.m
        ADDITIONAL_PATH $<TARGET_FILE_DIR:cmake_matlab_test_wrapper1>
        )
    set_tests_properties(${PROJECT_NAME}_matlabtest-1 PROPERTIES WILL_FAIL TRUE)

    # without the unit test framework
    matlab_add_unit_test(
        NAME ${PROJECT_NAME}_matlabtest-2
        TIMEOUT 300
        NO_UNITTEST_FRAMEWORK
        UNITTEST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../nonexistantfile2.m
        ADDITIONAL_PATH $<TARGET_FILE_DIR:cmake_matlab_test_wrapper1>
        )
    set_tests_properties(${PROJECT_NAME}_matlabtest-2 PROPERTIES WILL_FAIL TRUE)
endif()
