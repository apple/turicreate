project(test_include_dirs)
include(CTest)

enable_testing()

add_executable(dummy dummy.cpp)

function(generate_tests NAME)
  set(ctest_file "${CMAKE_CURRENT_BINARY_DIR}/${NAME}_tests.cmake")
  add_custom_command(
    OUTPUT "${ctest_file}"
    COMMAND "${CMAKE_COMMAND}"
            -D "TEST_EXECUTABLE=$<TARGET_FILE:dummy>"
            -D "TEST_SUITE=${NAME}"
            -D "TEST_NAMES=${ARGN}"
            -D "CTEST_FILE=${ctest_file}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/add-tests.cmake"
    VERBATIM
  )
  add_custom_target(${NAME}_tests ALL DEPENDS "${ctest_file}")
endfunction()

generate_tests(house dog cat)
generate_tests(farm cow pig)
generate_tests(zoo fox emu)

set_property(DIRECTORY PROPERTY TEST_INCLUDE_FILE "house_tests.cmake")
set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "farm_tests.cmake")
set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "zoo_tests.cmake")
