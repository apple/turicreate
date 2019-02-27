foreach(f
    "${RunCMake_TEST_BINARY_DIR}/relative-input-WARN.txt"
    "${RunCMake_TEST_BINARY_DIR}/relative-output-WARN.txt"
    )
  if(EXISTS "${f}")
    file(READ "${f}" content)
    if(NOT content MATCHES "^relative-input-WARN[\r\n]*$")
      string(APPEND RunCMake_TEST_FAILED "File\n  ${f}\ndoes not have expected content.\n")
    endif()
  else()
    string(APPEND RunCMake_TEST_FAILED "Missing\n  ${f}\n")
  endif()
endforeach()
