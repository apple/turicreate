include(RunCMake)

function(run_CleanByproducts case)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/CleanByproducts-${case}-build)
  set(RunCMake_TEST_OPTIONS "${ARGN}")

  run_cmake(CleanByproducts)
  set(RunCMake_TEST_NO_CLEAN 1)

  run_cmake_command(CleanByProducts-build ${CMAKE_COMMAND} --build .)
  include("${RunCMake_TEST_BINARY_DIR}/files.cmake")

  message("Checking that all expected files are present")
  check_files(EXPECTED_PRESENT "${RunCMake_TEST_BINARY_DIR}" TRUE)
  check_files(EXPECTED_DELETED "${RunCMake_TEST_BINARY_DIR}" TRUE)

  run_cmake_command(CleanByProducts-clean ${CMAKE_COMMAND} --build . --target clean)

  message("Checking that only the expected files are present after cleaning")
  check_files(EXPECTED_PRESENT "${RunCMake_TEST_BINARY_DIR}" TRUE)
  check_files(EXPECTED_DELETED "${RunCMake_TEST_BINARY_DIR}" FALSE)
endfunction()

function(check_files list path has_to_exist)
  foreach(file IN LISTS ${list})
    message("Checking ${file}")
    set(file_exists FALSE)
    if(EXISTS "${path}/${file}")
      set(file_exists TRUE)
    endif()

    if(file_exists AND NOT has_to_exist)
      message(FATAL_ERROR "${file} should have been deleted")
    elseif(NOT file_exists AND has_to_exist)
      message(FATAL_ERROR "${file} does not exist")
    elseif(file_exists AND has_to_exist)
      message("${file} found as expected")
    elseif(NOT file_exists AND NOT has_to_exist)
      message("${file} deleted as expected")
    endif()

  endforeach()
endfunction()


# Iterate through all possible test values
set(counter 0)
foreach(test_clean_no_custom TRUE FALSE)
  foreach(test_build_events TRUE FALSE)
    foreach(test_custom_command TRUE FALSE)
      foreach(test_custom_target TRUE FALSE)
        math(EXPR counter "${counter} + 1")
        message("Test ${counter} - CLEAN_NO_CUSTOM: ${test_clean_no_custom}, Build events: ${test_build_events}, Custom command: ${test_custom_command}, Custom target: ${test_custom_target}")
        run_CleanByproducts("buildevents${counter}" -DCLEAN_NO_CUSTOM=${test_clean_no_custom} -DTEST_BUILD_EVENTS=${test_build_events} -DTEST_CUSTOM_COMMAND=${test_custom_command} -DTEST_CUSTOM_TARGET=${test_custom_target})
      endforeach()
    endforeach()
  endforeach()
endforeach()
