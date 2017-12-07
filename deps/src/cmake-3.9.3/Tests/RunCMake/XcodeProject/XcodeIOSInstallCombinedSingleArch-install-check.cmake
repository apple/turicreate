function(verify_architecture file)
  execute_process(
    COMMAND xcrun lipo -info ${RunCMake_TEST_BINARY_DIR}/_install/${file}
    OUTPUT_VARIABLE lipo_out
    ERROR_VARIABLE lipo_err
    RESULT_VARIABLE lipo_result)
  if(NOT lipo_result EQUAL "0")
    message(SEND_ERROR "lipo -info failed: ${lipo_err}")
    return()
  endif()

  string(REGEX MATCHALL "is architecture: [^ \n\t]+" architecture "${lipo_out}")
  string(REGEX REPLACE "is architecture: " "" actual "${architecture}")

  set(expected armv7)

  if(NOT actual STREQUAL expected)
    message(SEND_ERROR
      "The actual library architecture:\n ${actual} \n"
      "which do not match expected ones:\n ${expected} \n"
      "lipo output:\n${lipo_out}")
  endif()
endfunction()

verify_architecture(lib/libfoo.dylib)
