
cmake_minimum_required (VERSION 2.8.12)
enable_testing()
project(component_checks)

set(MATLAB_FIND_DEBUG TRUE)

# the success of the following command is dependent on the current configuration:
# - on 32bits builds (cmake is building with 32 bits), it looks for 32 bits Matlab
# - on 64bits builds (cmake is building with 64 bits), it looks for 64 bits Matlab
find_package(Matlab REQUIRED COMPONENTS MX_LIBRARY ENG_LIBRARY MAT_LIBRARY MAIN_PROGRAM)

message(STATUS "FindMatlab libraries: ${Matlab_LIBRARIES}")

matlab_add_mex(
    # target name
    NAME cmake_matlab_test_wrapper1
    # output name
    OUTPUT_NAME cmake_matlab_mex1
    SRC ${CMAKE_CURRENT_SOURCE_DIR}/../matlab_wrapper1.cpp
    DOCUMENTATION ${CMAKE_CURRENT_SOURCE_DIR}/../help_text1.m.txt
    LINK_TO ${Matlab_LIBRARIES}
  )
